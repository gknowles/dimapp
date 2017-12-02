// Copyright Glen Knowles 2015 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//
// task.h - dim core
#pragma once

#include "cppconf/cppconf.h"

#include "core/handle.h"

#include <functional>
#include <string>

namespace Dim {


/****************************************************************************
*
*   Task queue
*
***/

struct TaskQueueHandle : HandleBase {};

class TaskQueue;

class ITaskNotify {
public:
    virtual ~ITaskNotify() = default;
    virtual void onTask() { delete this; }

private:
    friend class TaskQueue;
    ITaskNotify * m_taskNext{nullptr};
};

class TaskProxy : public ITaskNotify {
public:
    TaskProxy(std::function<void()> && fn) : m_fn{std::move(fn)} {}

private:
    void onTask() override { m_fn(); }

    std::function<void()> m_fn;
};


/****************************************************************************
*
*   Task API
*
***/

void taskPushEvent(ITaskNotify & task);
void taskPushEvent(ITaskNotify * tasks[], size_t numTasks);
TaskQueueHandle taskEventQueue();

void taskPushCompute(ITaskNotify & task);
void taskPushCompute(ITaskNotify * tasks[], size_t numTasks);
TaskQueueHandle taskComputeQueue();

TaskQueueHandle taskCreateQueue(std::string_view name, int threads);
void taskSetQueueThreads(TaskQueueHandle q, int threads);
void taskPush(TaskQueueHandle q, ITaskNotify & task);
void taskPush(TaskQueueHandle q, ITaskNotify * tasks[], size_t numTasks);

// Generally used for utility tasks the run for the life of the program
// such as the window message loop, service dispatcher, or timer queue.
//
// Creates a task queue with one thread task, runs the task, and then stops
// the thread. After the task completes the queue is leaked around until
// shutdown, so this shouldn't be used for anything that runs repeatedly.
void taskPushOnce(std::string_view name, std::function<void()> fn);

} // namespace
