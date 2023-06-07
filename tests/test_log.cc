#include "easy/base/LogAppender.h"
#include "easy/base/LogFormatter.h"
#include "easy/base/Logger.h"
#include "easy/base/common.h"

#include <unistd.h>
#include <iostream>
#include <memory>

void bench(const char* file, bool kLongLog)
{
  auto logger = std::make_shared<easy::Logger>("bench");

  easy::LogFormatter::ptr formatter = std::make_shared<easy::LogFormatter>("%m");

  easy::FileLogAppender::ptr fileAppender =
      std::make_shared<easy::FileLogAppender>(file);

  fileAppender->setFormatter(formatter);
  logger->addAppender(fileAppender);


  int64_t start = easy::util::timestamp();
  size_t n = 1000 * 1000;
  std::string content = "Hello 0123456789 abcdefghijklmnopqrstuvwxyz";
  std::string empty = " ";
  std::string longStr(3000, 'X');
  longStr += " ";
  size_t len = content.length() +
               (kLongLog ? longStr.length() : empty.length()) + sizeof(int);
  for (size_t i = 0; i < n; ++i)
  {
    EASY_LOG_DEBUG(logger) << content << (kLongLog ? longStr : empty) << i;
  }
  int64_t end = easy::util::timestamp();
  double seconds =
      static_cast<double>((end - start) / easy::util::kMicroSecondsPerSecond);
  printf("%12s:%f seconds, %ld bytes, %10.2f msg/s, %.2f MiB/s\n", file,
         seconds, n * len, static_cast<double>(n) / seconds,
         static_cast<double>(n * len) / seconds / (1024 * 1024));
}

void test_logger()
{
  auto logger = std::make_shared<easy::Logger>("root");
  logger->addAppender(std::make_shared<easy::ConsoleLogAppender>());

  easy::FileLogAppender::ptr fileAppender =
      std::make_shared<easy::FileLogAppender>("./test.log");
  logger->addAppender(fileAppender);

  EASY_LOG_TRACE(logger) << "trace";
  EASY_LOG_DEBUG(logger) << "debug";
  EASY_LOG_INFO(logger) << "info";
  EASY_LOG_WARN(logger) << "warn";
  EASY_LOG_ERROR(logger) << "error";
  EASY_LOG_FATAL(logger) << "fatal";

  EASY_LOG_FMT_TRACE(logger, "fmt trace: %s", "I am a formatter string");
  EASY_LOG_FMT_DEBUG(logger, "fmt debug: %s", "I am a formatter string");
  EASY_LOG_FMT_INFO(logger, "fmt info: %s", "I am a formatter string");
  EASY_LOG_FMT_WARN(logger, "fmt warn: %s", "I am a formatter string");
  EASY_LOG_FMT_ERROR(logger, "fmt error: %s", "I am a formatter string");
  EASY_LOG_FMT_FATAL(logger, "fmt fatal: %s", "I am a formatter string");

  logger->setLevel(easy::LogLevel::error);
  EASY_LOG_INFO(logger) << "this message never sink";

  auto l = easy::LoggerMgr::GetInstance()->getLogger("xx");
  EASY_LOG_INFO(l) << "xx logger";

  auto root = EASY_LOG_ROOT();
  EASY_LOG_INFO(root) << "root logger";
}

int main()
{
  test_logger();
  bench("/dev/null", true);
  bench("/tmp/log", true);
  bench("./bench.log", true);
  return 0;
}
