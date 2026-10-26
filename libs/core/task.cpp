// Copyright Glen Knowles 2015 - 2026.
// Distributed under the Boost Software License, Version 1.0.
//
// task.cpp - dim app
#include "pch.h"
#pragma hdrstop

using namespace std;
using namespace Dim;


/****************************************************************************
*
*   Incomplete public types
*
***/

class Dim::TaskQueue : public HandleContent {
public:
    static void push(TaskQueue * q, ITaskNotify & task);
    static void pop(TaskQueue * q);

public:
    TaskQueueHandle hq;
    string name;

    // Current threads have been created, haven't exited, but may not have
    // run yet.
    int curThreads{0};
    int wantThreads{0};

    ITaskNotify * first{};
    ITaskNotify * last{};

    condition_variable cv;
    mutex mut;
};


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

class EndThreadTask : public ITaskNotify {};

class FunctionTask : public ITaskNotify {
public:
    explicit FunctionTask(function<void()> && fn);
private:
    void onTask() override;
    function<void()> m_fn;
};

class RunOnceTask : public FunctionTask {
public:
    explicit RunOnceTask(string_view name, function<void()> && fn);
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static auto & s_perfThreads = iperf("task.threads");
static auto & s_perfDestroyed = iperf("task.shutdowns");
static auto & s_perfEnded = iperf("task.threads (ended)");

static TaskQueueHandle s_eventQ;
thread_local bool t_inEventThread;
static TaskQueueHandle s_computeQ;
static atomic_bool s_running;


/****************************************************************************
*
*   Queue Handles
*
***/

static HandleMap<TaskQueueHandle, TaskQueue> s_queues;
static shared_mutex s_qmut;
static condition_variable_any s_qdestroyed;

//===========================================================================
static TaskQueue * getQ(TaskQueueHandle hq) {
    shared_lock lk{s_qmut};
    return s_queues.find(hq);
}

//===========================================================================
static TaskQueueHandle setQ(TaskQueue * q) {
    unique_lock lk{s_qmut};
    q->hq = s_queues.insert(q);
    return q->hq;
}


/****************************************************************************
*
*   Run tasks
*
***/

//===========================================================================
static void taskQueueThread(TaskQueue * ptr) {
    iThreadInitialize();
    TaskQueue & q{*ptr};
    iThreadSetName(q.name);
    bool more{true};
    unique_lock lk{q.mut};
    while (more) {
        while (!q.first)
            q.cv.wait(lk);

        auto task = q.first;
        q.pop(&q);
        more = !dynamic_cast<EndThreadTask *>(task);
        lk.unlock();
        task->onTask();
        lk.lock();
    }
    q.curThreads -= 1;
    s_perfThreads -= 1;

    if (!s_perfThreads) {
        s_perfDestroyed += 1;
        lk.unlock();
        s_qdestroyed.notify_one();
    }
}

//===========================================================================
static void setThreads(TaskQueue * q, size_t threads) {
    unique_lock lk(q->mut);
    int num = (int) threads - q->wantThreads;
    q->wantThreads = (int)threads;
    if (num > 0) {
        q->curThreads += num;
        s_perfThreads += num;
        for (int i = 0; i < num; ++i) {
            thread thr{taskQueueThread, q};
            thr.detach();
        }
    } else if (num < 0) {
        for (int i = 0; i > num; --i) {
            s_perfEnded += 1;
            auto task = new EndThreadTask;
            q->push(q, *task);
        }
        q->cv.notify_all();
    }
}


/****************************************************************************
*
*   TaskQueue
*
***/

//===========================================================================
// static
void TaskQueue::push(TaskQueue * q, ITaskNotify & task) {
    task.m_taskNext = nullptr;
    if (!q->first) {
        q->first = &task;
    } else {
        q->last->m_taskNext = &task;
    }
    q->last = &task;
}

//===========================================================================
// static
void TaskQueue::pop(TaskQueue * q) {
    auto task = q->first;
    q->first = task->m_taskNext;
    task->m_taskNext = nullptr;
}


/****************************************************************************
*
*   FunctionTask
*
***/

//===========================================================================
FunctionTask::FunctionTask(function<void()> && fn)
    : m_fn{fn}
{}

//===========================================================================
void FunctionTask::onTask() {
    m_fn();
    delete this;
}


/****************************************************************************
*
*   RunOnceTask
*
***/

//===========================================================================
RunOnceTask::RunOnceTask(string_view name, function<void()> && fn)
    : FunctionTask{move(fn)}
{
    auto q = taskCreateQueue(name, 1);
    taskPush(q, this);
    taskSetQueueThreads(q, 0);
}


/****************************************************************************
*
*   Internal API
*
***/

//===========================================================================
void Dim::iTaskInitialize() {
    s_running = true;
    s_eventQ = taskCreateQueue("Event", 1);
    taskPush(s_eventQ, [](){ t_inEventThread = true; });
    s_computeQ = taskCreateQueue("Compute", 5);
}

//===========================================================================
void Dim::iTaskDestroy() {
    s_running = false;
    unique_lock lk{s_qmut};

    // send shutdown task to all task threads
    for (auto && q : s_queues)
        setThreads(q.second, 0);

    // wait for all threads to stop
    while (s_perfThreads)
        s_qdestroyed.wait(lk);

    // delete task queues
    for (auto && q : s_queues)
        s_queues.erase(q.first);
}


/****************************************************************************
*
*   Public API
*
***/

//===========================================================================
void Dim::taskPushEvent(function<void()> && fn) {
    taskPush(s_eventQ, move(fn));
}

//===========================================================================
void Dim::taskPushEvent(ITaskNotify * task) {
    taskPush(s_eventQ, task);
}

//===========================================================================
void Dim::taskPushEvent(ITaskNotify * tasks[], size_t numTasks) {
    taskPush(s_eventQ, tasks, numTasks);
}

//===========================================================================
TaskQueueHandle Dim::taskEventQueue() {
    return s_eventQ;
}

//===========================================================================
bool Dim::taskInEventThread() {
    return t_inEventThread;
}

//===========================================================================
void Dim::taskPushCompute(function<void()> && fn) {
    taskPush(s_computeQ, move(fn));
}

//===========================================================================
void Dim::taskPushCompute(ITaskNotify * task) {
    taskPush(s_computeQ, task);
}

//===========================================================================
void Dim::taskPushCompute(ITaskNotify * tasks[], size_t numTasks) {
    taskPush(s_computeQ, tasks, numTasks);
}

//===========================================================================
TaskQueueHandle Dim::taskComputeQueue() {
    return s_computeQ;
}

//===========================================================================
TaskQueueHandle Dim::taskCreateQueue(string_view name, int threads) {
    assert(s_running);
    assert(threads);
    auto q = new TaskQueue;
    q->name = name;

    auto hq = setQ(q);
    setThreads(q, threads);
    return hq;
}

//===========================================================================
void Dim::taskSetQueueThreads(TaskQueueHandle hq, int threads) {
    assert(s_running);

    auto q = getQ(hq);
    setThreads(q, threads);
}

//===========================================================================
void Dim::taskPush(TaskQueueHandle hq, function<void()> && fn) {
    taskPush(hq, new FunctionTask(move(fn)));
}

//===========================================================================
void Dim::taskPush(TaskQueueHandle hq, ITaskNotify * task) {
    ITaskNotify * list[] = {task};
    taskPush(hq, list, size(list));
}

//===========================================================================
void Dim::taskPush(
    TaskQueueHandle hq,
    ITaskNotify * tasks[],
    size_t numTasks
) {
    assert(s_running);

    if (!numTasks)
        return;

    auto q = getQ(hq);
    unique_lock lk{q->mut};
    for (auto i = 0; (size_t) i < numTasks; ++tasks, ++i)
        q->push(q, **tasks);

    if (numTasks > 1 && q->curThreads > 1) {
        q->cv.notify_all();
    } else {
        q->cv.notify_one();
    }
}

//===========================================================================
void Dim::taskPushOnce(string_view name, function<void()> && fn) {
    new RunOnceTask(name, move(fn));
}
