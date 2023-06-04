# easy

## 日志实现目标

- [x] `%m`：输出日志信息。
- [x] `%p`：输出日志级别。
- [x] `%r`：输出从应用程序启动到输出该 Log 事件所耗费的毫秒数。
- [ ] `%c`：输出所属的类目，通常就是所在类的全名。
- [x] `%t`：输出产生该日志事件的线程id。
- [x] `%n`：输出一个回车换行符。相当于换了一行。
- [x] `%d`：输出日志时间点的日期或时间，可以在其后指定格式，比如 %d{yyy-MM-dd HH:mm:ss.%06ld}。
- [x] `%F`：输出日志消息产生时所在的文件名称。
- [ ] `%x`：输出和当前线程关联的 NDC（嵌套诊断上下文）。
- [x] `%L`：输出代码中的行号。

## 参考链接
1. https://github.com/chenshuo/muduo
2. https://github.com/sylar-yin/sylar
3. https://github.com/gabime/spdlog