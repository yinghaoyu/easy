#include "easy/base/LogAppender.h"
#include "easy/base/LogFormatter.h"
#include "easy/base/Logger.h"

#include <unistd.h>
#include <iostream>
#include <memory>

void bench(const char* file, bool kLongLog)
{
    auto logger = std::make_shared<easy::Logger>("bench");

    easy::LogFormatter::ptr formatter = std::make_shared<easy::LogFormatter>("%m");  // only message

    easy::FileLogAppender::ptr fileAppender = std::make_shared<easy::FileLogAppender>(file);

    // easy::ConsoleLogAppender::ptr consoleAppender = std::make_shared<easy::ConsoleLogAppender>();

    fileAppender->setFormatter(formatter);
    logger->addAppender(fileAppender);

    // consoleAppender->setFormatter(formatter);
    // logger->addAppender(consoleAppender);

    size_t      n       = 1000 * 1000;
    std::string content = "Hello 0123456789 abcdefghijklmnopqrstuvwxyz";
    std::string empty   = " ";
    std::string longStr(3000, 'X');
    longStr += " ";
    size_t          len   = content.length() + (kLongLog ? longStr.length() : empty.length()) + sizeof(int);
    easy::Timestamp start = easy::Timestamp::now();
    for (size_t i = 0; i < n; ++i)
    {
        ELOG_DEBUG(logger) << content << (kLongLog ? longStr : empty) << i;
    }
    easy::Timestamp end     = easy::Timestamp::now();
    double          seconds = static_cast<double>(easy::timeDifference(end, start)) / 1000;
    printf("%12s:%f seconds, %ld bytes, %10.2f msg/s, %.2f MiB/s\n",
        file,
        seconds,
        n * len,
        static_cast<double>(n) / seconds,
        static_cast<double>(n * len) / seconds / (1024 * 1024));
}

void test_logger()
{
    auto logger = std::make_shared<easy::Logger>("root");
    logger->addAppender(std::make_shared<easy::ConsoleLogAppender>());

    easy::FileLogAppender::ptr fileAppender = std::make_shared<easy::FileLogAppender>("./test.log");
    logger->addAppender(fileAppender);

    ELOG_TRACE(logger) << "trace";
    ELOG_DEBUG(logger) << "debug";
    ELOG_INFO(logger) << "info";
    ELOG_WARN(logger) << "warn";
    ELOG_ERROR(logger) << "error";
    ELOG_FATAL(logger) << "fatal";

    ELOG_FMT_TRACE(logger, "fmt trace: %s", "I am a formatter string");
    ELOG_FMT_DEBUG(logger, "fmt debug: %s", "I am a formatter string");
    ELOG_FMT_INFO(logger, "fmt info: %s", "I am a formatter string");
    ELOG_FMT_WARN(logger, "fmt warn: %s", "I am a formatter string");
    ELOG_FMT_ERROR(logger, "fmt error: %s", "I am a formatter string");
    ELOG_FMT_FATAL(logger, "fmt fatal: %s", "I am a formatter string");

    logger->setLevel(easy::LogLevel::ERROR);
    ELOG_INFO(logger) << "this message never sink";

    auto l = easy::LoggerMgr::GetInstance()->getLogger("xx");
    ELOG_INFO(l) << "xx logger";

    auto root = ELOG_ROOT();
    ELOG_INFO(root) << "root logger";
}

int main()
{
    test_logger();
    bench("/dev/null", false);
    bench("/tmp/log", false);
    bench("./bench.log", false);
    return 0;
}
