# easy

## 日志

- [x] 支持不同种类的输出目标，比如标准输出、文件。
- [x] 支持类似 `log4j` 的输出格式配置。
  - [x] `%p`：输出日志级别。
  - [x] `%r`：输出从应用程序启动到输出该 Log 事件所耗费的毫秒数。
  - [ ] `%c`：输出所属的类目，通常就是所在类的全名。
  - [x] `%t`：输出产生该日志事件的线程id。
  - [x] `%n`：输出一个回车换行符。相当于换了一行。
  - [x] `%d`：输出日志时间点的日期或时间，可以在其后指定格式，比如 %d{yyy-MM-dd HH:mm:ss.%06ld}。
  - [x] `%F`：输出日志消息产生时所在的文件名称。
  - [ ] `%x`：输出和当前线程关联的 NDC（嵌套诊断上下文）。
  - [x] `%L`：输出代码中的行号。

## 协程及调度器

- [x] 协程基于 `ucontext_t cluster`。
- [x] 每个协程都使用独立的栈。
- [x] 线程的 `root` 协程可以不要栈空间，只负责调度，当然调度协程不一定得是 `root` 协程。
- [x] 调度器支持协程调度到其他线程。
- [x] 调度器采用简单的 `Round-robin` 策略。
- [x] 一个 `N * M` 的调度器，`n` 个线程，`m`个协程，协程可以切换到任意线程上。

## 参考链接

1. https://github.com/chenshuo/muduo
2. https://github.com/sylar-yin/sylar
3. https://github.com/gabime/spdlog
4. https://github.com/oceanbase/oceanbase
