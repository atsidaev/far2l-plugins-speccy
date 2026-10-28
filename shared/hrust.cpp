#include "hrust.hpp"
#include <algorithm>

namespace speccy {
  namespace {
    class HrustBits {
      Input input;
      unsigned bits = 0, index = 0;
      bool eof = false;

      void refill() {
        if(input.remaining() < 2) {
          eof = true;
          bits = 0;
          return;
        }
        bits = input.byte();
        bits |= unsigned(input.byte()) << 8;
      }

    public:
      HrustBits(const uint8_t * data, size_t size) : input(data, size) {
        refill();
      }

      uint8_t getByte() {
        return input.byte();
      }

      unsigned getBit() {
        if(eof)
          throw DecodeError();
        unsigned b = (bits >> (15 - index)) & 1;
        if(index == 15)
          refill();
        index = (index + 1) % 16;
        return b;
      }

      unsigned getBits(int n) {
        unsigned r = 0;
        while(n--)
          r = 2 * r + getBit();
        return r;
      }

      bool error() const {
        return eof;
      }
    };
  }

  bool hrust1(const uint8_t * src, size_t size, uint8_t * dest, size_t capacity, size_t & written) {
    written = 0;
    size_t offset = size >= 2 && src[0] == 'H' && src[1] == 'R' ? 0 : 0x103;
    if(size < offset + 12)
      return false;
    const auto * from = src + offset;
    if(from[0] != 'H' || from[1] != 'R')
      return false;
    size_t expected = le16(from + 2), packed = le16(from + 4);
    if(expected > capacity || expected < 7 || packed > size - offset)
      return false;
    try {
      HrustBits s(from + 12, std::min(packed, size - offset - 12));
      Output output(dest, expected);
      output.byte(s.getByte());
      uint8_t noBits = 2;
      uint8_t mask[] = {0, 0, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0};

      while(!s.error()) {
        while(s.getBit())
          output.byte(s.getByte());

        uint16_t len = 0;
        uint8_t bb = 0;
        do {
          bb = s.getBits(2);
          len += bb;
        } while(bb == 0x03 && len != 0x0f);

        short offset = 0;

        if(len == 0) {
          offset = 0xfff8 + s.getBits(3);
          output.copy(offset, 1);
          continue;
        }

        if(len == 1) {
          uint8_t code = s.getBits(2);

          if(code == 2) {
            uint8_t b = s.getByte();
            if(b >= 0xe0) {
              b <<= 1;
              ++b;    // rlca
              b ^= 2; // xor c

              if(b == 0xff) {
                if(++noBits > 8)
                  return false;
                continue;
              }

              offset = 0xff00 + b - 0x0f;

              output.copy(offset, 1);
              output.byte(s.getByte());
              output.copy(offset, 1);
              continue;
            }
            offset = 0xff00 + b;
          }

          if(code == 0 || code == 1) {
            offset = s.getByte();
            offset += 256 * (code ? 0xfe : 0xfd);
          }
          if(code == 3)
            offset = 0xffe0 + s.getBits(5);

          for(uint8_t i = 0; i < 2; ++i)
            output.copy(offset, 1);
          continue;
        }

        if(len == 3) {
          if(s.getBit()) {
            offset = 0xfff0 + s.getBits(4);
            output.copy(offset, 1);
            output.byte(s.getByte());
            output.copy(offset, 1);
            continue;
          }

          if(s.getBit()) {
            uint8_t noBytes = 6 + s.getBits(4);
            for(uint8_t i = 0; i < 2 * noBytes; ++i)
              output.byte(s.getByte());
            continue;
          }

          len = s.getBits(7);
          if(len == 0x0f)
            break; // EOF
          if(len < 0x0f)
            len = 256 * len + s.getByte();
        }

        if(len == 2)
          ++len;

        uint8_t code = s.getBits(2);

        if(code == 1) {
          uint8_t b = s.getByte();

          if(b >= 0xe0) {
            if(len > 3)
              return false;

            b <<= 1;
            ++b;    // rlca
            b ^= 3; // xor c

            offset = 0xff00 + b - 0x0f;

            output.copy(offset, 1);
            output.byte(s.getByte());
            output.copy(offset, 1);
            continue;
          }
          offset = 0xff00 + b;
        }

        if(code == 0)
          offset = 0xfe00 + s.getByte();
        if(code == 2)
          offset = 0xffe0 + s.getBits(5);
        if(code == 3) {
          offset = 256 * (mask[noBits] + s.getBits(noBits));
          offset += s.getByte();
        }

        for(uint16_t i = 0; i < len; ++i)
          output.copy(offset, 1);
      }

      for(int i = 0; i < 6; ++i)
        output.byte(from[6 + i]);

      written = output.size();
      return written == expected;
    } catch(const DecodeError &) {
      return false;
    }
  }
}
