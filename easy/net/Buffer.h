#ifndef __EASY_BUFFER_H__
#define __EASY_BUFFER_H__

#include <stdint.h>
#include <sys/socket.h>  // iovec
#include <memory>
#include <string>
#include <vector>

namespace easy {
class Buffer {
 public:
  typedef std::shared_ptr<Buffer> ptr;

  struct Node {
    Node(size_t s);
    Node();
    ~Node();

    void free();

    char* ptr_;
    Node* next_;
    size_t size_;
  };

  Buffer(size_t base_size = 4096);

  Buffer(void* data, size_t size, bool owner = false);

  ~Buffer();

  // write
  void writeFint8(int8_t value);
  void writeFuint8(uint8_t value);
  void writeFint16(int16_t value);
  void writeFuint16(uint16_t value);
  void writeFint32(int32_t value);
  void writeFuint32(uint32_t value);
  void writeFint64(int64_t value);
  void writeFuint64(uint64_t value);

  void writeInt32(int32_t value);
  void writeUint32(uint32_t value);
  void writeInt64(int64_t value);
  void writeUint64(uint64_t value);

  void writeFloat(float value);
  void writeDouble(double value);
  // length:int16 , data
  void writeStringF16(const std::string& value);
  // length:int32 , data
  void writeStringF32(const std::string& value);
  // length:int64 , data
  void writeStringF64(const std::string& value);
  // length:varint, data
  void writeStringVint(const std::string& value);
  // data
  void writeStringWithoutLength(const std::string& value);

  // read
  int8_t readFint8();
  uint8_t readFuint8();
  int16_t readFint16();
  uint16_t readFuint16();
  int32_t readFint32();
  uint32_t readFuint32();
  int64_t readFint64();
  uint64_t readFuint64();

  int32_t readInt32();
  uint32_t readUint32();
  int64_t readInt64();
  uint64_t readUint64();

  float readFloat();
  double readDouble();

  // length:int16, data
  std::string readStringF16();
  // length:int32, data
  std::string readStringF32();
  // length:int64, data
  std::string readStringF64();
  // length:varint , data
  std::string readStringVint();

  void clear();

  void write(const void* buf, size_t size);
  void read(void* buf, size_t size);
  void read(void* buf, size_t size, size_t position) const;

  size_t position() const { return position_; }
  void setPosition(size_t v);

  bool writeToFile(const std::string& name, bool with_md5 = false) const;
  bool readFromFile(const std::string& name);

  size_t baseSize() const { return baseSize_; }
  size_t readSize() const { return size_ - position_; }

  bool isLittleEndian() const;
  void setIsLittleEndian(bool val);

  std::string toString() const;
  std::string toHexString() const;

  // 只获取内容，不修改position
  uint64_t getReadBuffers(std::vector<iovec>& buffers,
                          uint64_t len = ~0ull) const;
  uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len,
                          uint64_t position) const;
  // 增加容量，不修改position
  uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

  size_t size() const { return size_; }

 private:
  void addCapacity(size_t size);
  size_t getCapacity() const { return capacity_ - position_; }

 private:
  size_t baseSize_;
  size_t position_;
  size_t capacity_;
  size_t size_;
  int8_t endian_;  // 1小端 2大端
  bool owner_;
  Node* root_;
  Node* cur_;
};
}  // namespace easy

#endif
