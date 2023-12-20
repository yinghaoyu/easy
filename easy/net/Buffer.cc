#include "easy/net/Buffer.h"
#include "easy/base/HashUtil.h"
#include "easy/base/Logger.h"
#include "easy/net/Endian.h"

#include <math.h>  // ceil
#include <memory.h>
#include <cstdint>
#include <fstream>
#include <iomanip>

namespace easy {
static Logger::ptr logger = EASY_LOG_NAME("system");

Buffer::Node::Node(size_t s) : ptr_(new char[s]), next_(nullptr), size_(s) {}

Buffer::Node::Node() : ptr_(nullptr), next_(nullptr), size_(0) {}

Buffer::Node::~Node() {}

void Buffer::Node::free() {
  if (ptr_) {
    delete[] ptr_;
    ptr_ = nullptr;
  }
}

Buffer::Buffer(size_t base_size)
    : baseSize_(base_size),
      position_(0),
      capacity_(base_size),
      size_(0),
      endian_(EASY_BIG_ENDIAN),
      owner_(true),
      root_(new Node(base_size)),
      cur_(root_) {}

Buffer::Buffer(void* data, size_t size, bool owner)
    : baseSize_(size),
      position_(0),
      capacity_(size),
      size_(size),
      endian_(EASY_BIG_ENDIAN),
      owner_(owner) {
  root_ = new Node();
  root_->ptr_ = static_cast<char*>(data);
  root_->size_ = size;
  cur_ = root_;
}

Buffer::~Buffer() {
  Node* tmp = root_;
  while (tmp) {
    cur_ = tmp;
    tmp = tmp->next_;
    if (owner_) {
      cur_->free();
    }
    delete cur_;
  }
}

bool Buffer::isLittleEndian() const { return endian_ == EASY_LITTLE_ENDIAN; }

void Buffer::setIsLittleEndian(bool val) {
  if (val) {
    endian_ = EASY_LITTLE_ENDIAN;
  } else {
    endian_ = EASY_BIG_ENDIAN;
  }
}

void Buffer::writeFint8(int8_t value) { write(&value, sizeof(value)); }

void Buffer::writeFuint8(uint8_t value) { write(&value, sizeof(value)); }

void Buffer::writeFint16(int16_t value) {
  if (endian_ != EASY_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void Buffer::writeFuint16(uint16_t value) {
  if (endian_ != EASY_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void Buffer::writeFint32(int32_t value) {
  if (endian_ != EASY_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void Buffer::writeFuint32(uint32_t value) {
  if (endian_ != EASY_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void Buffer::writeFint64(int64_t value) {
  if (endian_ != EASY_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void Buffer::writeFuint64(uint64_t value) {
  if (endian_ != EASY_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v) {
  if (v < 0) {
    return (static_cast<uint32_t>(-v)) * 2 - 1;
  } else {
    return static_cast<uint32_t>(v * 2);
  }
}

static uint64_t EncodeZigzag64(const int64_t& v) {
  if (v < 0) {
    return (static_cast<uint64_t>(-v)) * 2 - 1;
  } else {
    return static_cast<uint64_t>(v * 2);
  }
}

static int32_t DecodeZigzag32(const uint32_t& v) { return (v >> 1) ^ -(v & 1); }

static int64_t DecodeZigzag64(const uint64_t& v) { return (v >> 1) ^ -(v & 1); }

void Buffer::writeInt32(int32_t value) { writeUint32(EncodeZigzag32(value)); }

void Buffer::writeUint32(uint32_t value) {
  uint8_t tmp[5];
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = static_cast<uint8_t>(static_cast<uint8_t>(value & 0x7F) | 0x80);
    value >>= 7;
  }
  tmp[i++] = static_cast<uint8_t>(value);
  write(tmp, i);
}

void Buffer::writeInt64(int64_t value) { writeUint64(EncodeZigzag64(value)); }

void Buffer::writeUint64(uint64_t value) {
  uint8_t tmp[10];
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = static_cast<uint8_t>(static_cast<uint8_t>(value & 0x7F) | 0x80);
    value >>= 7;
  }
  tmp[i++] = static_cast<uint8_t>(value);
  write(tmp, i);
}

void Buffer::writeFloat(float value) {
  uint32_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint32(v);
}

void Buffer::writeDouble(double value) {
  uint64_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint64(v);
}

void Buffer::writeStringF16(const std::string& value) {
  writeFuint16(static_cast<uint16_t>(value.size()));
  write(value.c_str(), value.size());
}

void Buffer::writeStringF32(const std::string& value) {
  writeFuint32(static_cast<uint32_t>(value.size()));
  write(value.c_str(), value.size());
}

void Buffer::writeStringF64(const std::string& value) {
  writeFuint64(value.size());
  write(value.c_str(), value.size());
}

void Buffer::writeStringVint(const std::string& value) {
  writeUint64(value.size());
  write(value.c_str(), value.size());
}

void Buffer::writeStringWithoutLength(const std::string& value) {
  write(value.c_str(), value.size());
}

int8_t Buffer::readFint8() {
  int8_t v;
  read(&v, sizeof(v));
  return v;
}

uint8_t Buffer::readFuint8() {
  uint8_t v;
  read(&v, sizeof(v));
  return v;
}

#define XX(type)                    \
  type v;                           \
  read(&v, sizeof(v));              \
  if (endian_ == EASY_BYTE_ORDER) { \
    return v;                       \
  } else {                          \
    return byteswap(v);             \
  }

int16_t Buffer::readFint16() { XX(int16_t); }

uint16_t Buffer::readFuint16() { XX(uint16_t); }

int32_t Buffer::readFint32() { XX(int32_t); }

uint32_t Buffer::readFuint32() { XX(uint32_t); }

int64_t Buffer::readFint64() { XX(int64_t); }

uint64_t Buffer::readFuint64() { XX(uint64_t); }

#undef XX

int32_t Buffer::readInt32() { return DecodeZigzag32(readUint32()); }

uint32_t Buffer::readUint32() {
  uint32_t result = 0;
  for (int i = 0; i < 32; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= (static_cast<uint32_t>(b) << i);
      break;
    } else {
      result |= (static_cast<uint32_t>(b & 0x7f) << i);
    }
  }
  return result;
}

int64_t Buffer::readInt64() { return DecodeZigzag64(readUint64()); }

uint64_t Buffer::readUint64() {
  uint64_t result = 0;
  for (int i = 0; i < 64; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= (static_cast<uint64_t>(b) << i);
      break;
    } else {
      result |= (static_cast<uint64_t>(b & 0x7f) << i);
    }
  }
  return result;
}

float Buffer::readFloat() {
  uint32_t v = readFuint32();
  float value;
  memcpy(&value, &v, sizeof(v));
  return value;
}

double Buffer::readDouble() {
  uint64_t v = readFuint64();
  double value;
  memcpy(&value, &v, sizeof(v));
  return value;
}

std::string Buffer::readStringF16() {
  uint16_t len = readFuint16();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

std::string Buffer::readStringF32() {
  uint32_t len = readFuint32();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

std::string Buffer::readStringF64() {
  uint64_t len = readFuint64();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

std::string Buffer::readStringVint() {
  uint64_t len = readUint64();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

void Buffer::clear() {
  position_ = size_ = 0;
  capacity_ = baseSize_;
  Node* tmp = root_->next_;
  while (tmp) {
    cur_ = tmp;
    tmp = tmp->next_;
    if (owner_) {
      cur_->free();
    }
    delete cur_;
  }
  cur_ = root_;
  root_->next_ = NULL;
}

void Buffer::write(const void* buf, size_t size) {
  if (size == 0) {
    return;
  }
  // 尝试扩容
  addCapacity(size);

  size_t npos = position_ % baseSize_;  // 节点内偏移
  size_t ncap = cur_->size_ - npos;     // 当前节点的剩余大小
  size_t bpos = 0;  // 在 buf 内的偏移  while (size > 0) {
  if (ncap >= size) {
    // 写长度 <= 节点剩余大小
    memcpy(cur_->ptr_ + npos, static_cast<const char*>(buf) + bpos, size);
    if (cur_->size_ == (npos + size)) {
      // 当前节点写完了，切换到下一块
      cur_ = cur_->next_;
    }
    position_ += size;
    bpos += size;
    size = 0;
  } else {
    // 写长度 > 节点剩余大小
    memcpy(cur_->ptr_ + npos, static_cast<const char*>(buf) + bpos, ncap);
    position_ += ncap;
    bpos += ncap;
    size -= ncap;
    cur_ = cur_->next_;
    ncap = cur_->size_;
    npos = 0;
  }
  if (position_ > size_) {
    // 扩容过，更新一下 array 大小
    size_ = position_;
  }
}

void Buffer::read(void* buf, size_t size) {
  if (size > readSize()) {
    throw std::out_of_range("not enough len");
  }

  size_t npos = position_ % baseSize_;  // 在节点内的偏移
  size_t ncap = cur_->size_ - npos;     // 节点剩余大小
  size_t bpos = 0;                      // 在 buf 的偏移
  while (size > 0) {
    if (ncap >= size) {
      // 读取长度 <= 节点剩余大小
      memcpy(static_cast<char*>(buf) + bpos, cur_->ptr_ + npos, size);
      if (cur_->size_ == (npos + size)) {
        // 当前节点读完了，切换到下一块
        cur_ = cur_->next_;
      }
      position_ += size;
      bpos += size;
      size = 0;
    } else {  // 读取长度 > 节点剩余大小
      memcpy(static_cast<char*>(buf) + bpos, cur_->ptr_ + npos, ncap);
      position_ += ncap;
      bpos += ncap;
      size -= ncap;
      cur_ = cur_->next_;
      ncap = cur_->size_;
      npos = 0;
    }
  }
}
void Buffer::read(void* buf, size_t size, size_t position) const {
  if (size > readSize()) {
    throw std::out_of_range("not enough len");
  }

  size_t npos = position % baseSize_;
  size_t ncap = cur_->size_ - npos;
  size_t bpos = 0;
  Node* cur = cur_;
  while (size > 0) {
    if (ncap >= size) {
      memcpy(static_cast<char*>(buf) + bpos, cur->ptr_ + npos, size);
      if (cur->size_ == (npos + size)) {
        cur = cur->next_;
      }
      position += size;
      bpos += size;
      size = 0;
    } else {
      memcpy(static_cast<char*>(buf) + bpos, cur->ptr_ + npos, ncap);
      position += ncap;
      bpos += ncap;
      size -= ncap;
      cur = cur->next_;
      ncap = cur->size_;
      npos = 0;
    }
  }
}
void Buffer::setPosition(size_t v) {
  if (v > capacity_) {
    throw std::out_of_range("set_position out of range");
  }
  position_ = v;
  if (position_ > size_) {
    size_ = position_;
  }
  cur_ = root_;
  while (v > cur_->size_) {
    v -= cur_->size_;
    cur_ = cur_->next_;
  }
  if (v == cur_->size_) {
    cur_ = cur_->next_;
  }
}
bool Buffer::writeToFile(const std::string& name, bool with_md5) const {
  std::ofstream ofs;
  ofs.open(name, std::ios::trunc | std::ios::binary);
  if (!ofs) {
    EASY_LOG_ERROR(logger) << "writeToFile name=" << name
                           << " error , errno=" << errno
                           << " errstr=" << strerror(errno);
    return false;
  }

  size_t read_size = readSize();
  size_t pos = position_;
  Node* cur = cur_;

  while (read_size > 0) {
    size_t diff = pos % baseSize_;
    size_t len = (read_size > baseSize_ ? baseSize_ : read_size) - diff;
    ofs.write(cur->ptr_ + diff, static_cast<long>(len));
    cur = cur->next_;
    pos += len;
    read_size -= len;
  }
  if (with_md5) {
    std::ofstream ofs_md5(name + ".md5");
    ofs_md5 << getMd5();
  }

  return true;
}

bool Buffer::readFromFile(const std::string& name) {
  std::ifstream ifs;
  ifs.open(name, std::ios::binary);
  if (!ifs) {
    EASY_LOG_ERROR(logger) << "readFromFile name=" << name
                           << " error, errno=" << errno
                           << " errstr=" << strerror(errno);
    return false;
  }

  std::shared_ptr<char> buff(new char[baseSize_], [](char* p) { delete[] p; });
  while (!ifs.eof()) {
    ifs.read(buff.get(), static_cast<long>(baseSize_));
    write(buff.get(), static_cast<size_t>(ifs.gcount()));
  }
  return true;
}
void Buffer::addCapacity(size_t size) {
  if (size == 0) {
    return;
  }
  size_t old_cap = getCapacity();
  if (old_cap >= size) {
    return;
  }
  // 需要扩容
  size = size - old_cap;
  // 扩容需要的节点数
  size_t count = static_cast<size_t>(
      ceil(static_cast<double>(size) / static_cast<double>(baseSize_)));
  Node* tmp = root_;
  while (tmp->next_) {
    tmp = tmp->next_;
  }
  // 把扩容的节点加入链表
  Node* first = NULL;
  for (size_t i = 0; i < count; ++i) {
    tmp->next_ = new Node(baseSize_);
    if (first == NULL) {
      first = tmp->next_;
    }
    tmp = tmp->next_;
    capacity_ += baseSize_;
  }

  if (old_cap == 0) {
    cur_ = first;
  }
}

std::string Buffer::toString() const {
  std::string str;
  str.resize(readSize());
  if (str.empty()) {
    return str;
  }
  read(&str[0], str.size(), position_);
  return str;
}

std::string Buffer::toHexString() const {
  std::string str = toString();
  std::stringstream ss;

  for (size_t i = 0; i < str.size(); ++i) {
    if (i > 0 && i % 32 == 0) {
      ss << std::endl;
    }
    ss << std::setw(2) << std::setfill('0') << std::hex << str[i] << " ";
  }
  return ss.str();
}
uint64_t Buffer::getReadBuffers(std::vector<iovec>& buffers,
                                uint64_t len) const {
  len = len > readSize() ? readSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = position_ % baseSize_;
  size_t ncap = cur_->size_ - npos;
  struct iovec iov;
  Node* cur = cur_;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr_ + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr_ + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next_;
      ncap = cur->size_;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}
uint64_t Buffer::getReadBuffers(std::vector<iovec>& buffers, uint64_t len,
                                uint64_t position) const {
  len = len > readSize() ? readSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = position % baseSize_;
  size_t count = position / baseSize_;
  Node* cur = root_;
  while (count > 0) {
    cur = cur->next_;
    --count;
  }
  size_t ncap = cur->size_ - npos;
  struct iovec iov;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr_ + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr_ + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next_;
      ncap = cur->size_;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

uint64_t Buffer::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
  if (len == 0) {
    return 0;
  }
  addCapacity(len);
  uint64_t size = len;

  size_t npos = position_ % baseSize_;
  size_t ncap = cur_->size_ - npos;
  struct iovec iov;
  Node* cur = cur_;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr_ + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr_ + npos;
      iov.iov_len = ncap;

      len -= ncap;
      cur = cur->next_;
      ncap = cur->size_;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

std::string Buffer::getMd5() const {
  std::vector<iovec> buffers;
  getReadBuffers(buffers, -1UL, 0);
  return easy::md5sum(buffers);
}
}  // namespace easy
