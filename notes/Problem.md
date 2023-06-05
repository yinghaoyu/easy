# 问题记录

1. `std::enable_shared_from_this` 一定要用 `public` 继承，否则使用`shared_from_this`会抛出 `std::bad_weak_ptr`。

2. `#include` 的作用等于 `copy`、`paste`。

3. 获取线程 `id` 时，可以用线程局部变量缓存，下次获取时走缓存，可以减少系统调用。

4. 线程类非常需要 `start` 和 `stop` 这两个函数，以便测试并发，刚开始没有设计这两个函数，导致线程谁先构造，谁的 `cb` 就先运行了。

5. 当要监控某个变量时，打印是很蠢的，可能漏打印，可以使用 `gdb` 的 `watch`、`rwatch`。

6. 协程切换时，一定要清楚上下文的位置。
