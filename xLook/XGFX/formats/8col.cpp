#include <windows.h>
#include "common.hpp"
#include "../format.hpp"
#include "../image.hpp"

namespace theX
{

void putPixel( int row, int col, int pos, BYTE grb, BGR* pic )
{
  BYTE x = pos & 7;
  BYTE y = (pos>>3) & 7;
  int  i = 256*(8*row+y) + (8*col+x);

  if( grb & 2 ) pic[i].r = 0xff;
  if( grb & 4 ) pic[i].g = 0xff;
  if( grb & 1 ) pic[i].b = 0xff;
}

class BB8colStream
{
  private:
    const BYTE* p;
    BYTE  index;
    BYTE  bits;
  public:
    BB8colStream(const BYTE* start) { p = start; index = 8; }

    BYTE getByte( void ) { return *p++; }
    BYTE getBit ( void )
    {
      BYTE mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
      if( index == 8 ) { bits = getByte(); index = 0; }
      BYTE result = bits&mask[index++] ? 1 : 0;
      return result;
    }
    BYTE getBits( int n )
    {
      BYTE result = 0;
      for( int i = 0; i < n; ++i ) result = (result << 1) | getBit();
      return result;
    }
    void getPalette( BYTE* palette, int n )
    {
      for( int i = 0; i < n; ++i ) palette[n-i-1] = getBits( 3 );
    }
};

class _8col888 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->ext[0] == '8'&& hdr->ext[1] == '8' && hdr->ext[2] == '8') return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "8col .888";

      Page* pg = im->addPage();
      if(!pg) return 0;

      BB8colStream bb(body);

      BYTE palette[8], lastType;

      for( WORD row = 0; row < 24; ++row )
        for( WORD col = 0; col < 32; ++col )
        {
          BYTE type = bb.getBits(3);
          if( type == 0 )
          {
            for( int line = 0; line < 8; ++line )
            {
              BYTE red   = bb.getByte();
              BYTE green = bb.getByte();
              BYTE blue  = bb.getByte();

              BYTE mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
              for( int i = 0; i < 8; ++i )
              {
                BYTE r = (red   & mask[i]) ? 0x02 : 0;
                BYTE g = (green & mask[i]) ? 0x04 : 0;
                BYTE b = (blue  & mask[i]) ? 0x01 : 0;
                putPixel( row, col, 8*line+i, (r|g|b), pg->pix );
              }
            }
            continue;
          }

          if( type == 1 )
            type = lastType;
          else
            bb.getPalette( palette, type-1 );
          for( int i = 0; i < 64; ++i )
          {
            BYTE index = 0;
            switch( type )
            {
              case 3:
                index = bb.getBit();
                break;
              case 4:
                index = bb.getBit();
                if( index != 0 ) index = bb.getBit() + 1;
                break;
              case 5:
                index = bb.getBits(2);
                break;
              case 6:
                index = bb.getBits(2);
                if( index == 3 ) index = ((index<<1) | bb.getBit())-3;
                break;
              case 7:
                index = bb.getBits(2);
                if( index > 1 ) index = ((index<<1) | bb.getBit())-2;
                break;
            }
            putPixel( row, col, i, palette[index], pg->pix );
          }
          lastType = type;
        }
      return im;
    }
} __8col888__;

BGR getColor(BYTE b)
{
  BGR pixel;
  pixel.r = (b & 2) ? 0xff : 0;
  pixel.g = (b & 4) ? 0xff : 0;
  pixel.b = (b & 1) ? 0xff : 0;
  return pixel;
}

// 8 colors [byte/pix (128x128 or 128x96 00000aaa)] [BRG]
class _8col_1in1 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      size_t lines = 0;
      if(hdr && hdr->length == 0x4000 && hdr->noSex == 64) lines = 128;
      if(hdr && hdr->length == 0x3000 && hdr->noSex == 48) lines = 96;

      if(lines == 0 ) return 0;

      for( int i = 0; i < 128*lines; ++i )
        if(body[i] & 0xf8) return 0;

      return 9;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;
      im->format = "8col pix/byte";

      size_t lines = 96;
      if(hdr->length == 0x4000) lines = 128;

      Page* pg = im->addPage(128, lines);
      if(pg)
      {
        for(int i = 0; i < 128*lines; ++i)
          pg->pix[i] = getColor(body[i]);
      }
      return im;
    }
} __8col_1in1__;

// 8 colors [byte/2pix (256x128 or 256x192 0lll0rrr)] [BRG]
class _8col_2in1 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      size_t lines = 0;
      if(hdr && hdr->length == 24576 && hdr->noSex == 96) lines = 192;
      if(hdr && hdr->length == 0x4000 && hdr->noSex == 64) lines = 128;

      if(lines == 0 ) return 0;

      for(size_t i = 0; i < 256*lines/2; ++i)
        if(body[i] & 0x88) return 0;

      return 9;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;
      im->format = "8col 2pix/byte";

      size_t lines = 128;
      if(hdr->length == 24576) lines = 192;

      Page* pg = im->addPage(256, lines);
      if(pg)
      {
        for(size_t i = 0; i < 256*lines/2; ++i)
        {
          pg->pix[2*i] = getColor((body[i] & 0x70) >> 4);
          pg->pix[2*i+1] = getColor(body[i] & 0x07);
        }
      }
      return im;
    }
} ___8col_2in1__;
}
