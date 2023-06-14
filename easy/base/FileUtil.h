#ifndef __EASY_FILEUTIL_H__
#define __EASY_FILEUTIL_H__

#include <iostream>
#include <string>
#include <vector>

namespace easy
{

class FileUtil
{
 public:
  static void ListAllFile(std::vector<std::string> &files,
                          const std::string &path,
                          const std::string &subfix);

  static bool Mkdir(const std::string &dirname);

  static bool IsRunningPidfile(const std::string &pidfile);

  static bool Rm(const std::string &path);

  static bool Mv(const std::string &from, const std::string &to);

  static bool Realpath(const std::string &path, std::string &rpath);

  static bool Symlink(const std::string &frm, const std::string &to);

  static bool Unlink(const std::string &filename, bool exist = false);

  static std::string Dirname(const std::string &filename);

  static std::string Basename(const std::string &filename);

  static bool OpenForRead(std::ifstream &ifs,
                          const std::string &filename,
                          std::ios_base::openmode mode = std::ios_base::in);

  static bool OpenForWrite(std::ofstream &ofs,
                           const std::string &filename,
                           std::ios_base::openmode mode = std::ios_base::out);
};

}  // namespace easy

#endif
