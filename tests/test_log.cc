#include "easy/base/easy_define.h"

#include <unistd.h>
#include <iostream>
#include <memory>

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
  return 0;
}
