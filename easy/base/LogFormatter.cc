#include "easy/base/LogFormatter.h"
#include "easy/base/Logger.h"

#include <functional>
#include <map>
#include <string>

namespace easy
{
LogFormatter::LogFormatter(const std::string &pattern) : pattern_(pattern)
{
  compile_pattern(pattern);
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level,
                                 LogEvent::ptr event)
{
  std::stringstream ss;
  for (auto &i : items_)
  {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

std::ostream &LogFormatter::format(std::ostream &ofs,
                                   std::shared_ptr<Logger> logger,
                                   LogLevel::Level level,
                                   LogEvent::ptr event)
{
  for (auto &i : items_)
  {
    i->format(ofs, logger, level, event);
  }
  return ofs;
}

void LogFormatter::compile_pattern(const std::string &pattern)
{
  // str, format, type
  std::vector<std::tuple<std::string, std::string, int>> vec;
  std::string nstr;
  for (size_t i = 0; i < pattern.size(); ++i)
  {
    if (pattern[i] != '%')
    {
      nstr.append(1, pattern[i]);
      continue;
    }

    if ((i + 1) < pattern.size())
    {
      if (pattern[i + 1] == '%')
      {
        nstr.append(1, '%');
        continue;
      }
    }

    size_t n = i + 1;
    int fmt_status = 0;
    size_t fmt_begin = 0;

    std::string str;
    std::string fmt;
    while (n < pattern.size())
    {
      if (!fmt_status &&
          (!isalpha(pattern[n]) && pattern[n] != '{' && pattern[n] != '}'))
      {
        str = pattern.substr(i + 1, n - i - 1);
        break;
      }
      if (fmt_status == 0)
      {
        if (pattern[n] == '{')
        {
          str = pattern.substr(i + 1, n - i - 1);
          fmt_status = 1;  // 解析格式
          fmt_begin = n;
          ++n;
          continue;
        }
      }
      else if (fmt_status == 1)
      {
        if (pattern[n] == '}')
        {
          fmt = pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          fmt_status = 0;
          ++n;
          break;
        }
      }
      ++n;
      if (n == pattern.size())
      {
        if (str.empty())
        {
          str = pattern.substr(i + 1);
        }
      }
    }

    if (fmt_status == 0)
    {
      if (!nstr.empty())
      {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
        nstr.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n - 1;
    }
    else if (fmt_status == 1)
    {
      error_ = true;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    }
  }
  if (!nstr.empty())
  {
    vec.push_back(std::make_tuple(nstr, "", 0));
  }
  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string &str)>>
      s_format_items = {
#define XX(str, C)                                        \
  {                                                       \
    #str, [](const std::string &fmt)                      \
    { return FormatItem::ptr(std::make_shared<C>(fmt)); } \
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
  for (auto &i : vec)
  {
    if (std::get<2>(i) == 0)
    {
      items_.push_back(std::make_shared<StringFormatItem>(std::get<0>(i)));
    }
    else
    {
      auto it = s_format_items.find(std::get<0>(i));
      if (it == s_format_items.end())
      {
        items_.push_back(std::make_shared<StringFormatItem>(
            "<<error_format %" + std::get<0>(i) + ">>"));
        error_ = true;
      }
      else
      {
        items_.push_back(it->second(std::get<1>(i)));
      }
    }
  }
}

}  // namespace easy
