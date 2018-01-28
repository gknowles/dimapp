// Copyright Glen Knowles 2015 - 2018.
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
    TaskQueueHandle hq;
    string name;

    // current threads have been created, haven't exited, but may not have
    // run yet.
    int curThreads{0};
    int wantThreads{0};

    ITaskNotify * first{nullptr};
    ITaskNotify * last{nullptr};

    condition_variable cv;

    void push(ITaskNotify & task);
    void pop();
};


/****************************************************************************
*
*   Private declarations
*
***/

namespace {

class EndThreadTask : public ITaskNotify {};

class RunOnceTask : public ITaskNotify {
public:
    explicit RunOnceTask(string_view name, function<void()> && fn);

private:
    void onTask() override;

    function<void()> m_fn;
};

} // namespace


/****************************************************************************
*
*   Variables
*
***/

static HandleMap<TaskQueueHandle, TaskQueue> s_queues;
static int s_numThreads;
static mutex s_mut;
static condition_variable s_destroyed;
static int s_numDestroyed;
static int s_numEnded;

static TaskQueueHandle s_eventQ;
static TaskQueueHandle s_computeQ;
static atomic_bool s_running;


/****************************************************************************
*
*   Run tasks
*
***/

//===========================================================================
static void taskQueueThread(TaskQueue * ptr) {
    TaskQueue & q{*ptr};
    iThreadSetName(q.name);
    bool more{true};
    unique_lock<mutex> lk{s_mut};
    while (more) {
        while (!q.first)
            q.cv.wait(lk);

        auto * task = q.first;
        q.pop();
        more = !dynamic_cast<EndThreadTask *>(task);
        lk.unlock();
        task->onTask();
        lk.lock();
    }
    q.curThreads -= 1;
    s_numThreads -= 1;

    if (!s_numThreads) {
        s_numDestroyed += 1;
        lk.unlock();
        s_destroyed.notify_one();
    }
}

//===========================================================================
static void setThreads_LK(TaskQueue & q, size_t threads) {
    int num = (int) threads - q.wantThreads;
    q.wantThreads = (int)threads;
    if (num > 0) {
        q.curThreads += num;
        s_numThreads += num;
        for (int i = 0; i < num; ++i) {
            thread thr{taskQueueThread, &q};
            thr.detach();
        }
    } else if (num < 0) {
        for (int i = 0; i > num; --i) {
            s_numEnded += 1;
            auto * task = new EndThreadTask;
            q.push(*task);
        }
        q.cv.notify_all();
    }
}


/****************************************************************************
*
*   TaskQueue
*
***/

//===========================================================================
void TaskQueue::push(ITaskNotify & task) {
    task.m_taskNext = nullptr;
    if (!first) {
        first = &task;
    } else {
        last->m_taskNext = &task;
    }
    last = &task;
}

//===========================================================================
void TaskQueue::pop() {
    auto * task = first;
    first = task->m_taskNext;
    task->m_taskNext = nullptr;
}


/****************************************************************************
*
*   RunOnceTask
*
***/

//===========================================================================
RunOnceTask::RunOnceTask(string_view name, function<void()> && fn)
    : m_fn{fn}
{
    auto q = taskCreateQueue(name, 1);
    taskPush(q, this);
    taskSetQueueThreads(q, 0);
}

//===========================================================================
void RunOnceTask::onTask() {
    m_fn();
    delete this;
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
    s_computeQ = taskCreateQueue("Compute", 5);
}

//===========================================================================
void Dim::iTaskDestroy() {
    s_running = false;
    unique_lock<mutex> lk{s_mut};

    // send shutdown task to all task threads
    for (auto && q : s_queues)
        setThreads_LK(*q.second, 0);

    // wait for all threads to stop
    while (s_numThreads)
        s_destroyed.wait(lk);

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
void Dim::taskPushEvent(ITaskNotify * task) {
    ITaskNotify * list[] = {task};
    taskPushEvent(list, size(list));
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
void Dim::taskPushCompute(ITaskNotify * task) {
    ITaskNotify * list[] = {task};
    taskPushCompute(list, size(list));
}

//===========================================================================
void Dim::taskPushCompute(ITaskNotify * tasks[], size_t numTasks) {
    Dim::taskPush(s_computeQ, tasks, numTasks);
}

//===========================================================================
TaskQueueHandle Dim::taskComputeQueue() {
    return s_computeQ;
}

//===========================================================================
TaskQueueHandle Dim::taskCreateQueue(string_view name, int threads) {
    assert(s_running);
    assert(threads);
    auto * q = new TaskQueue;
    q->name = name;

    scoped_lock<mutex> lk{s_mut};
    q->hq = s_queues.insert(q);
    setThreads_LK(*q, threads);
    return q->hq;
}

//===========================================================================
void Dim::taskSetQueueThreads(TaskQueueHandle hq, int threads) {
    assert(s_running);

    scoped_lock<mutex> lk{s_mut};
    auto * q = s_queues.find(hq);
    setThreads_LK(*q, threads);
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

    scoped_lock<mutex> lk{s_mut};
    auto * q = s_queues.find(hq);
    for (auto i = 0; (size_t) i < numTasks; ++tasks, ++i)
        q->push(**tasks);

    if (numTasks > 1 && q->curThreads > 1) {
        q->cv.notify_all();
    } else {
        q->cv.notify_one();
    }
}

//===========================================================================
void Dim::taskPushOnce(string_view name, function<void()> fn) {
    new RunOnceTask(name, move(fn));
}
