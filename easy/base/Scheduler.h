#ifndef __EASY_SCHEDULER_H__
#define __EASY_SCHEDULER_H__

#include "easy/base/Atomic.h"
#include "easy/base/Fiber.h"
#include "easy/base/Mutex.h"
#include "easy/base/Thread.h"
#include "easy/base/noncopyable.h"

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace easy
{
// N-M asymmetic Scheduler
class Scheduler : noncopyable
{
    friend class Fiber;

  public:
    Scheduler(int threadNums = 1, bool use_caller = true, const std::string& name = "");

    virtual ~Scheduler();

    const std::string& name() const { return name_; }

    void start();

    void stop();

    virtual bool canStop();

    bool hasIdleThread() const { return idleThreadNums_.get() > 0; }

    void run();

    template <typename T>
    void schedule(T co, int threadId = -1)
    {
        bool should_weakup = false;
        {
            WriteLockGuard _(lock_);
            should_weakup = scheduleNonBlock(co, threadId);
        }
        if (should_weakup)
        {
            weakup();
        }
    }

    template <typename TaskIterator>
    void schedule(TaskIterator begin, TaskIterator end)
    {
        bool need_weakup = false;
        {
            WriteLockGuard _(lock_);
            while (begin != end)
            {
                need_weakup = scheduleNonBlock(*begin) || need_weakup;
                ++begin;
            }
        }
        if (need_weakup)
        {
            weakup();
        }
    }

    static Scheduler* GetThis();

    static Fiber* GetSchedulerFiber();

  private:
    virtual void weakup();

    void handleFiber(Fiber::ptr& fiber);

    virtual void idle();

    template <typename T>
    bool scheduleNonBlock(T&& fiber, long thread_id = -1)
    {
        bool should_weakup = tasks_.empty();
        auto task          = std::make_shared<Task>(std::forward<T>(fiber), thread_id);
        if (task->fiber_ || task->cb_)
        {
            tasks_.push_back(std::move(task));
        }
        return should_weakup;
    }

  private:
    struct Task
    {
        typedef std::shared_ptr<Task> ptr;
        Task() {}

        Task(const Task& rhs) = default;

        Task(Fiber::ptr fiber, int threadId) : fiber_(fiber), threadId_(threadId) {}

        Task(const std::function<void()>& cb, int threadId) : cb_(cb), threadId_(threadId) {}

        Task(const std::function<void()>&& cb,
            int                            threadId) noexcept  // noexcept for std::move
            : cb_(std::move(cb)), threadId_(threadId)
        {}

        Task& operator=(const Task& other) = default;

        void reset()
        {
            fiber_    = nullptr;
            cb_       = nullptr;
            threadId_ = -1;
        }

        Fiber::ptr            fiber_;
        std::function<void()> cb_;
        int                   threadId_{-1};  // -1 could be scheduled by any thread
    };

    Scheduler::Task::ptr take();

  private:
    std::string      name_;                 // 调度器名
    int              callerTid_{-1};        // use_caller only
    std::vector<int> threadIds_;            // 线程 id 列表
    size_t           threadNums_{0};        // 线程池的线程数
    AtomicInt<int>   activeThreadNums_{0};  // 活跃线程数
    AtomicInt<int>   idleThreadNums_{0};    // 空闲线程数
    bool             running_{false};       // 执行状态

  private:
    ReadWriteLock            lock_;
    std::vector<Thread::ptr> threads_;      // 线程对象列表
    std::list<Task::ptr>     tasks_;        // 任务集合
    Fiber::ptr               callerFiber_;  // use_caller only
};

}  // namespace easy

#endif
