#pragma once
#include "bytes.hpp"
#include <algorithm>

namespace speccy {
  namespace png_detail {
    inline void big32(Bytes & bytes, uint32_t value) {
      bytes.push_back(value >> 24);
      bytes.push_back(value >> 16);
      bytes.push_back(value >> 8);
      bytes.push_back(value);
    }

    inline uint32_t crc(const uint8_t * data, size_t size) {
      uint32_t value = 0xffffffff;
      while(size--) {
        value ^= *data++;
        for(int bit = 0; bit < 8; ++bit)
          value = (value >> 1) ^ ((value & 1) ? 0xedb88320u : 0);
      }
      return ~value;
    }

    inline void chunk(Bytes & png, const char * type, const Bytes & data) {
      big32(png, data.size());
      size_t start = png.size();
      png.insert(png.end(), type, type + 4);
      png.insert(png.end(), data.begin(), data.end());
      big32(png, crc(png.data() + start, 4 + data.size()));
    }
  }

  inline bool pngFromBgr(const uint8_t * pixels, size_t width, size_t height, Bytes & result) {
    if(!pixels || !width || !height || width > 16384 || height > 16384)
      return false;
    size_t stride = 1 + width * 3, size = stride * height;
    if(size > 64 * 1024 * 1024)
      return false;
    Bytes rows;
    rows.reserve(size);
    uint32_t a = 1, b = 0;
    auto byte = [&](uint8_t value) {
      rows.push_back(value);
      a = (a + value) % 65521;
      b = (b + a) % 65521;
    };
    for(size_t y = 0; y < height; ++y) {
      byte(0);
      for(size_t x = 0; x < width; ++x, pixels += 3) {
        byte(pixels[2]);
        byte(pixels[1]);
        byte(pixels[0]);
      }
    }
    Bytes zlib = {0x78, 0x01};
    for(size_t offset = 0; offset < rows.size();) {
      uint16_t length = static_cast<uint16_t>(std::min(rows.size() - offset, size_t(65535)));
      zlib.push_back(offset + length == rows.size() ? 1 : 0);
      put16(zlib, length);
      put16(zlib, static_cast<uint16_t>(~length));
      zlib.insert(zlib.end(), rows.begin() + offset, rows.begin() + offset + length);
      offset += length;
    }
    png_detail::big32(zlib, (b << 16) | a);
    Bytes png = {137, 80, 78, 71, 13, 10, 26, 10}, header;
    png_detail::big32(header, width);
    png_detail::big32(header, height);
    header.insert(header.end(), {8, 2, 0, 0, 0});
    png_detail::chunk(png, "IHDR", header);
    png_detail::chunk(png, "IDAT", zlib);
    png_detail::chunk(png, "IEND", {});
    result = std::move(png);
    return true;
  }
}
