#ifndef __EASY_SCHEDULER_H__
#define __EASY_SCHEDULER_H__

#include "easy/base/Atomic.h"
#include "easy/base/Coroutine.h"
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
  friend class Coroutine;

 public:
  Scheduler(int threadNums = 1,
            bool use_caller = true,
            const std::string &name = "");

  virtual ~Scheduler();

  const std::string &getName() const { return name_; }

  void start();

  void stop();

  virtual bool canStop();

  bool hasIdleThread() const { return idleThreadNums_.get() > 0; }

  void run();

  template <typename T>
  void schedule(T co, int threadId = -1)
  {
    bool should_tickle = false;
    {
      WriteLockGuard lock(lock_);
      should_tickle = scheduleNonBlock(co, threadId);
    }
    if (should_tickle)
    {
      tickle();
    }
  }

  template <typename TaskIterator>
  void schedule(TaskIterator begin, TaskIterator end)
  {
    bool need_tickle = false;
    {
      WriteLockGuard lock(lock_);
      while (begin != end)
      {
        need_tickle = scheduleNonBlock(*begin) || need_tickle;
        ++begin;
      }
    }
    if (need_tickle)
    {
      tickle();
    }
  }

  static Scheduler *GetThis();

  static Coroutine *GetSchedulerCoroutine();

 private:
  virtual void tickle();

  void handleCoroutine(Coroutine::ptr& co);

  virtual void idle();

  template <typename T>
  bool scheduleNonBlock(T &&co, long thread_id = -1)
  {
    bool should_tickle = tasks_.empty();
    auto task = std::make_shared<Task>(std::forward<T>(co), thread_id);
    if (task->co_ || task->cb_)
    {
      tasks_.push_back(std::move(task));
    }
    return should_tickle;
  }

 private:
  struct Task
  {
    typedef std::shared_ptr<Task> ptr;
    Task() {}

    Task(const Task &rhs) = default;

    Task(Coroutine::ptr co, int threadId) : co_(co), threadId_(threadId) {}

    Task(const std::function<void()> &cb, int threadId)
        : cb_(cb), threadId_(threadId)
    {
    }

    Task(const std::function<void()> &&cb,
         int threadId) noexcept  // noexcept for std::move
        : cb_(std::move(cb)), threadId_(threadId)
    {
    }

    Task &operator=(const Task &other) = default;

    void reset()
    {
      co_ = nullptr;
      cb_ = nullptr;
      threadId_ = -1;
    }

    Coroutine::ptr co_;
    std::function<void()> cb_;
    int threadId_{-1};  // -1 could be scheduled by any thread
  };

  Scheduler::Task::ptr take();

 private:
  std::string name_;                    // 调度器名
  int callerTid_{-1};                   // use_caller only
  std::vector<int> threadIds_;          // 线程 id 列表
  size_t threadNums_{0};                // 线程池的线程数
  AtomicInt<int> activeThreadNums_{0};  // 活跃线程数
  AtomicInt<int> idleThreadNums_{0};    // 空闲线程数
  bool running_{false};                 // 执行状态

 private:
  RWLock lock_;
  std::vector<Thread::ptr> threads_;  // 线程对象列表
  std::list<Task::ptr> tasks_;        // 任务集合
  Coroutine::ptr callerCo_;           // use_caller only
};

}  // namespace easy

#endif
