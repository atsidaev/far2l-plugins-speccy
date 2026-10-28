#include <windows.h>
#include "common.hpp"
#include "../format.hpp"
#include "../image.hpp"

namespace theX
{

WORD getrealadr(WORD virtadr, WORD lb, WORD sc)
{
  WORD realadr;
  if (virtadr < lb)
  {
    WORD Lin =   (virtadr & 0x0007) << 8;
    WORD Ryd =   (virtadr & 0x0038) << 2;
    WORD Stolb = (virtadr & 0x07C0) >> 6;
    WORD Sg =     virtadr & 0x1800;
    realadr = Sg | Lin | Ryd | Stolb;
  } else {
    realadr = virtadr + sc;
  }

  return realadr;
}

void unpack_lc40(const BYTE* source, BYTE* destination)
{
  for( int i = 0;    i < 6144; ++i ) destination[i] = 0;
  for( int i = 6144; i < 6912; ++i ) destination[i] = 7;

  const BYTE* p = source;
  BYTE depacker[] = { 0x0e, 0xf9, 0x0d, 0xcd, 0xc6, 0x1f, 0x11, 0x90, 0x00, 0x19, 0x11, 0x00 };

  if(checkSignature(source, depacker)) p += 0x95;

  WORD sc = (*p++) << 8;

  BYTE *res = destination;

  WORD to = (sc & 0x0300) << 3;
  WORD lb = (sc ^ 0x1800) & 0xFC00;

  while( *p != 0xc0 )
  {
    BYTE flags = *p++;
    if( (flags&0xc1) == 0xc1 ) // rle
    {
      // rle
      BYTE len = (flags & 0x3c)>>2;
      if( !len ) len = *p++; else if( (flags&0x02) == 0x00 ) ++len;

      BYTE what = 0;
      if( (flags&0x02) == 0x00 ) what = *p++;
      do { res[getrealadr(to++,lb,sc)] = what; } while( --len );
    }
    else if( (flags&0xc1) == 0xc0) // copy BYTEs
    {
      BYTE len = (flags&0x3e)>>1;
      do { res[getrealadr(to++,lb,sc)] = *p++; } while( --len );
    }
    else  // backreference
    {
      BYTE len  = (flags&0x78) >> 3;
      BYTE high = (flags&0x07) | 0xf8;

      if( len == 0 )
      {
        high <<= 1;
        high |= *p&0x01;
        len = *p++ >> 1;
      }

      ++len;
      WORD offset = (high << 8) + (*p++);
      WORD from = to + offset;

      int dir = flags&0x80 ? -1 : 1;
      do
      {
        res[getrealadr(to++,lb,sc)] = res[getrealadr(from,lb,sc)];
        from += dir;
      } while( --len );
    }
  }
}

class LC52Stream
{
  private:
    const BYTE* base;
    const BYTE* p;
    int   idx;
    int   len;
    bool  eof;
    BYTE  bits;
  public:
    LC52Stream(const BYTE* from, int blockSize)
    {
      base = p = from;
      len  = blockSize;
      idx  = 0;
      eof  = false;
    }

    BYTE getByte( void )
    {
      if( p - base > len ) { eof = true; return 0; }
      return *p++;
    }

    BYTE getBit( void )
    {
      BYTE mask[] = { 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 };
      if( idx == 0 ) bits = getByte();

      BYTE bit = (bits & mask[idx]) == 0 ? 0 : 1;

      idx = (idx + 1) % 8;
      return bit;
    }

    BYTE getCode( void )
    {
      BYTE code = 0xFE;

      for( int i = 0; i < 3; ++i )
      {
        if( getBit() ) return code + 1;
        code = 2*code + getBit();
      }
      return (2*code + getBit()) + 9;
    }

    BYTE getLen(void)
    {
      BYTE len = getCode();
      if( len == 0x100 - 7 )
      {
        len = getByte();
        --len;
      }
      else
        if( len > 0x100 - 7 ) --len;

      return len;
    }
    bool error( void ) { return eof; }
};

void unpack_lc52(const BYTE* source, BYTE* destination, WORD size)
{
  for( int i = 0;    i < 6144; ++i ) destination[i] = 0;
  for( int i = 6144; i < 6912; ++i ) destination[i] = 7;

  WORD msc[] = {0x0000, 0x100, 0x200, 0x800, 0x900, 0x1000};

  BYTE lcmp5[] = { 'L','C','M','P','5' };

  const BYTE* from = 0;
  WORD  sc   = 0;

  if(checkSignature(source, lcmp5))
  {
    from = source + source[7] + 9 ;
    sc = 256*from[-1];
  }
  else
  {
    sc = 256*source[0x91];
    from = source + 0x9a;
  }

  WORD to = (sc & 0x0300) << 3;
  WORD lb = (sc ^ 0x1800) & 0xFC00;

  LC52Stream s( from, size );

  BYTE* res = destination;

  res[getrealadr(to++, lb, sc)] = s.getByte();

  while( !s.error() )
  {
    if (s.getBit())
    {
      res[getrealadr(to++, lb, sc)] = s.getByte();
    }
    else
    {
      BYTE len = s.getLen();

      if( len == 0xFF ) break;

      WORD dist = s.getCode() << 8;

      BYTE napr = s.getBit();

      dist |=  s.getByte();

      len = -len;
      dist =  -dist;
      if (dist > 768) ++len;

      WORD from = to-dist;
      do
      {
        res[getrealadr(to++, lb, sc)] = res[getrealadr(from, lb, sc)];
        from += napr ? -1 : 1;
        --len;
      } while( len > 0 );
    }
  }
}


class LaserCompact52 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      BYTE lcmp5[] = {'L', 'C', 'M', 'P', '5'};
      if(checkSignature(body, lcmp5)) return 9;

      BYTE depacker[] = { 0xd9, 0x11, 0x8e, 0x00, 0x1c, 0xcd, 0xc6, 0x1f, 0x19, 0x7e, 0x23, 0xdd, 0x2e, 0xff };
      if(size >= 164 && checkSignature(body + 3, depacker)) return 9;

      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      BYTE  scrBuf[6912];
      unpack_lc52(body, scrBuf, size);
      Image* im = scr2image(scrBuf);
      im->format = "lc 5.2";
      return im;
    }
} __LaserCompact52__;

class LaserCompact40 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(!hdr) return 0;
      if(size < hdr->length - 1) return 0;
      if(body[hdr->length-1] != 0xc0) return 0;

      const BYTE* p = body;
      if(hdr->length > 0x95)
      {
        BYTE depacker[] = { 0x0e, 0xf9, 0x0d, 0xcd, 0xc6, 0x1f, 0x11, 0x90, 0x00, 0x19, 0x11, 0x00 };
        if(checkSignature(body, depacker)) p += 0x95;
      }
      BYTE msc[]  = { 0x0, 0x1, 0x2, 0x8, 0x9, 0x10 };
      for(int i = 0; i < sizeof(msc); ++i) if(*p == msc[i]) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      BYTE  scrBuf[6912];
      unpack_lc40(body, scrBuf);
      Image* im = scr2image(scrBuf);
      im->format = "lc 4.0";
      return im;
    }
} __LaserCompact40__;
}
