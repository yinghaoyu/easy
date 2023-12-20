#include "easy/base/HashUtil.h"

#include <openssl/md5.h>

namespace easy {
std::string md5sum(const std::vector<iovec>& data) {
  MD5_CTX ctx;
  MD5_Init(&ctx);
  for (auto& i : data) {
    MD5_Update(&ctx, i.iov_base, i.iov_len);
  }
  std::string result;
  result.resize(MD5_DIGEST_LENGTH);
  MD5_Final(reinterpret_cast<unsigned char*>(&result[0]), &ctx);
  return result;
}
}  // namespace easy
