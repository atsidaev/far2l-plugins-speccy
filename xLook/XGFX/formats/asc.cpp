#include <windows.h>
#include "common.hpp"
#include "../format.hpp"
#include "../image.hpp"

namespace theX
{

bool decrash(const BYTE* source, int packed_size, BYTE* dest)
{
  BYTE buf[6912];

  const BYTE* from = source;
  BYTE* to   = buf;

  while(*from != 0x80)
  {
    BYTE b = *from++;
    if(from - source > packed_size) return false;

    switch(b & 0xc0)
    {
      case 0x80: // copy
      {
        BYTE len = (b & 0x3f);
        if(to - buf > 6912 - len) return false;
        if(from - source > packed_size - len) return false;

        for(BYTE i = 0; i < len; ++i) *to++ = *from++;
        break;
      }
      case 0xc0: // RLE
      {
        BYTE len = (b & 0x3f)+3;
        if(to - buf > 6912 - len) return false;

        for(BYTE i = 0; i < len; ++i) *to++ = *from;
        ++from;
        break;
      }
      default: // backreference (LZ)
      {
        BYTE len    = ((b & 0xf8)>>3) + 3;
        WORD offset = (256*(b&0x07)) + *from++;
        if(to - buf > 6912 - len) return false;
        if(to - offset < buf) return false;

        for(BYTE i = 0; i < len; ++i) *to++ = *(to-offset);
      }
    }
  }

  if(from - source == packed_size - 1 && to - buf == 6912) {
    // copy attributes first
    for(int i = 6144; i < 6912; ++i) dest[i] = buf[i];

    // decode pixel data from 'by column' layout to
    // speccy format
    for(int segment = 0; segment < 3; ++segment) {
      for(int x = 0; x < 32; ++x) {
        for(int y = 0; y < 64; ++y) {
          int line = (y % 8) << 8;
          int row  = ((y / 8) % 8) << 5;

          dest[(segment<<11) | line | row | x] = buf[2048*segment + 64*x + y];
        }
      }
    }

    return true;
  } else {
    return false;
  }
}


class ScreenCrasher : public Format
{
  private:
    BYTE  scrBuf[6912];
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(!hdr) return 0;
      if(size < hdr->length - 1) return 0;
      if(body[hdr->length-1] != 0x80) return 0;
      if(hdr->length > 6912) return 0;

      const BYTE* p = body;
      int packed_size = hdr->length;

      if(hdr->length > 0xc2)
      {
        BYTE depacker[] = { 0xf3, 0xcd, 0x52, 0x00, 0x3b, 0x3b, 0xc1, 0x21, 0x97, 0x00, 0x09, 0xeb, 0x21, 0x66, 0x00, 0x09, 0x73, 0x23, 0x72 };
        if(checkSignature(body, depacker)) {
          p += 0xc2;
          packed_size -= 0xc2;
        }
      }
      if(decrash(p, packed_size, scrBuf)) return 9;

      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = scr2image(scrBuf);
      im->format = "Screen Crasher by ASC";
      return im;
    }
} __ScreenCrasher__;
}
