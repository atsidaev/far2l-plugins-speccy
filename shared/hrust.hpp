#pragma once
#include "bytes.hpp"

namespace speccy {

  inline bool hrust2(const uint8_t * src, size_t size, uint8_t * dest, size_t capacity,
                     size_t & written) {
    written = 0;
    if(size < 7)
      return false;
    try {
      Input input(src + 6, size - 6);
      Output output(dest, capacity);
      unsigned bits = 0, left = 0;
      auto bit = [&]() -> unsigned {
        if(!left) {
          bits = input.byte();
          left = 8;
        }
        unsigned b = (bits >> 7) & 1;
        bits <<= 1;
        --left;
        return b;
      };
      auto distance = [&]() -> int {
        uint16_t value;
        if(bit())
          value = 0xff00 | input.byte();
        else {
          value = 0xffff;
          unsigned high = bit();
          unsigned low = bit();
          unsigned count = 4 - (2 * high + low);
          while(count--)
            value = (value << 1) + bit() - 1;
          if(value == 0xffe1)
            value = input.byte();
          value = (value << 8) | input.byte();
        }
        return int(value) - 65536;
      };
      output.byte(input.byte());
      for(;;) {
        if(bit()) {
          output.byte(input.byte());
          continue;
        }
        unsigned len = 1, add;
        do {
          unsigned high = bit();
          add = 2 * high + bit();
          len += add;
        } while(add == 3 && len != 16);
        int offset = -1;
        if(len == 4) {
          if(bit()) {
            len = input.byte();
            if(!len)
              break;
            if(len < 16)
              len = (len << 8) | input.byte();
            offset = distance();
          } else {
            len = 0;
            for(int i = 0; i < 4; ++i)
              len = 2 * len + bit();
            len = 2 * (len + 6);
            while(len--)
              output.byte(input.byte());
            continue;
          }
        } else {
          if(len > 4)
            --len;
          if(len == 1) {
            unsigned value = 0;
            for(int i = 0; i < 3; ++i)
              value = 2 * value + bit();
            offset = int(value) - 8;
          } else if(len == 2)
            offset = int(input.byte()) - 256;
          else
            offset = distance();
        }
        output.copy(offset, len);
      }
      for(int i = 0; i < 6; ++i)
        output.byte(src[i]);
      written = output.size();
      return true;
    } catch(const DecodeError &) {
      return false;
    }
  }

  bool hrust1(const uint8_t * src, size_t size, uint8_t * dest, size_t capacity, size_t & written);
}
