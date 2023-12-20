#ifndef __EASY_HASHUTIL_H__
#define __EASY_HASHUTIL_H__

#include <sys/socket.h>
#include <string>
#include <vector>

namespace easy {
std::string md5sum(const std::vector<iovec>& data);
}  // namespace easy

#endif
