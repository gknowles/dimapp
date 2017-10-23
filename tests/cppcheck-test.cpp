#include <string_view>
#include <functional>
using namespace std;

class ITaskNotify {
public:
    virtual ~ITaskNotify() = default;
    virtual void onTask() { delete this; }

private:
    friend class TaskQueue;
    ITaskNotify * m_taskNext{nullptr};
};

class RunOnceTask : public ITaskNotify {
public:
    RunOnceTask(string_view name, function<void()> && fn);

private:
    void onTask() override;

    function<void()> m_fn;
};
