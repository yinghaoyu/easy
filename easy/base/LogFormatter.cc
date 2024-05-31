#include "easy/base/LogFormatter.h"
#include "easy/base/Logger.h"

#include <functional>
#include <map>
#include <string>

namespace easy {
LogFormatter::LogFormatter(const std::string& pattern) : pattern_(pattern) {
  compile(pattern);
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level, LogEvent::ptr event) {
  std::stringstream ss;
  for (auto& i : items_) {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs,
                                   std::shared_ptr<Logger> logger,
                                   LogLevel::Level level, LogEvent::ptr event) {
  for (auto& i : items_) {
    i->format(ofs, logger, level, event);
  }
  return ofs;
}

enum STATUS {
  UNIT,
  STR,
};

void LogFormatter::compile(const std::string& pattern) {
  size_t n = pattern.size();

  STATUS status = STR;

  std::string A;
  std::string B;

  std::vector<std::tuple<std::string, std::string, STATUS>> units;

  for (size_t i = 0; i < n; ++i) {
    switch (status) {
      case STR: {
        if (pattern[i] == '%') {
          status = UNIT;
        } else {
          A = pattern[i];
          units.emplace_back(std::make_tuple(std::move(A), std::move(B), STR));
        }
      } break;
      case UNIT: {
        A = pattern[i];
        if (i + 1 < n && pattern[i + 1] == '{') {
          size_t pos = pattern.find('}', i + 1);
          if (pos != std::string::npos) {
            B = pattern.substr(i + 2, pos - i - 2);
            i = pos;
          };
        }
        units.emplace_back(std::make_tuple(std::move(A), std::move(B), UNIT));
        status = STR;
      } break;
      default:
        error_ = true;
        break;
    }
  }

  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string& str)>>
      s_format_units = {
#define XX(str, C)                                      \
  {                                                     \
    #str, [](const std::string& fmt) {                  \
      return FormatItem::ptr(std::make_shared<C>(fmt)); \
    }                                                   \
  }
        XX(m, MessageFormatItem),      // m:消息
        XX(p, LevelFormatItem),        // p:日志级别
        XX(r, ElapseFormatItem),       // r:累计毫秒数
        XX(c, NameFormatItem),         // c:日志名称
        XX(t, ThreadIdFormatItem),     // t:线程id
        XX(n, NewLineFormatItem),      // n:换行
        XX(d, DateTimeFormatItem),     // d:时间
        XX(F, FilenameFormatItem),     // F:文件名
        XX(L, LineFormatItem),         // L:行号
        XX(T, TabFormatItem),          // T:Tab
        XX(b, SpaceFormatItem),        // b:空格
        XX(C, CoroutineIdFormatItem),  // C:协程id
        XX(N, ThreadNameFormatItem),   // N:线程名称
#undef XX
      };

  for (auto& unit : units) {
    if (std::get<2>(unit) == STR) {
      items_.emplace_back(
          std::make_shared<StringFormatItem>(std::get<0>(unit)));
    } else {
      auto it = s_format_units.find(std::get<0>(unit));
      if (it == s_format_units.end()) {
        items_.emplace_back(std::make_shared<StringFormatItem>(
            "<<error_format %" + std::get<0>(unit) + ">>"));
        error_ = true;
      } else {
        items_.push_back(it->second(std::get<1>(unit)));
      }
    }
  }
}

}  // namespace easy
