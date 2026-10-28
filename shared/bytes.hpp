#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace speccy {
  using Bytes = std::vector<uint8_t>;

  struct DecodeError : std::runtime_error {
    DecodeError() : std::runtime_error("Invalid or truncated packed data") {
    }
  };

  inline uint16_t le16(const uint8_t * p) {
    return p[0] | (uint16_t(p[1]) << 8);
  }

  inline uint32_t le32(const uint8_t * p) {
    return le16(p) | (uint32_t(le16(p + 2)) << 16);
  }

  inline void put16(Bytes & b, uint16_t n) {
    b.push_back(n);
    b.push_back(n >> 8);
  }

  inline void put32(Bytes & b, uint32_t n) {
    put16(b, n);
    put16(b, n >> 16);
  }

  class Input {
    const uint8_t * data_;
    size_t size_, pos_ = 0;

  public:
    Input(const uint8_t * data, size_t size) : data_(data), size_(size) {
    }

    size_t remaining() const {
      return size_ - pos_;
    }

    uint8_t byte() {
      if(!remaining())
        throw DecodeError();
      return data_[pos_++];
    }
  };

  class Output {
    uint8_t * data_;
    size_t capacity_, pos_ = 0;

  public:
    Output(uint8_t * data, size_t capacity) : data_(data), capacity_(capacity) {
    }

    size_t size() const {
      return pos_;
    }

    void byte(uint8_t b) {
      if(pos_ == capacity_)
        throw DecodeError();
      data_[pos_++] = b;
    }

    void copy(int offset, size_t count) {
      if(offset >= 0 || size_t(-offset) > pos_ || count > capacity_ - pos_)
        throw DecodeError();
      while(count--) {
        auto b = data_[pos_ - size_t(-offset)];
        byte(b);
      }
    }
  };

  inline uint16_t crc16(const uint8_t * data, size_t size) {
    uint16_t crc = 0;
    while(size--) {
      crc ^= uint16_t(*data++) << 8;
      for(int i = 0; i < 8; ++i)
        crc = (crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0);
    }
    return crc;
  }

  inline uint32_t sum(const uint8_t * data, size_t size) {
    uint32_t n = 0;
    while(size--)
      n += *data++;
    return n;
  }

  inline uint16_t hobetaChecksum(const uint8_t * header) {
    return 257 * sum(header, 15) + 105;
  }

  inline bool hobeta(const uint8_t * data, size_t size) {
    return size >= 17 && size >= 17 + size_t(data[14]) * 256 &&
           le16(data + 15) == hobetaChecksum(data);
  }
}
