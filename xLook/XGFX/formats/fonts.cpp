#include <windows.h>
#include "common.hpp"
#include "../format.hpp"
#include "../image.hpp"

namespace theX
{

void font2image(Image* im, const BYTE* from, WORD noChars)
{
  BYTE w = noChars >= 32 ? 32 : noChars;
  BYTE h = noChars/32 + (noChars%32 ? 1 : 0);

  Page* pg = im->addPage(8*w, 8*h);
  if(!pg) return;

  for( BYTE y = 0;  y < h; ++y )
    for( BYTE x = 0; x < w; ++x )
      for( BYTE l = 0; l < 8; ++l )
      {
        BYTE b = from[8*(w*y+x)+l];
        for( int i = 0; i < 8; ++i )
        {
          int index = 64*w*y + 8*x + 8*l*w + i;
          if( b & mask[i] )
            pg->pix[index] = black;
          else
            pg->pix[index] = white;
        }
      }
}

void font2image4x8(Image* im, const BYTE* from)
{
  Page*  pg = im->addPage(256, 64);
  if(!pg) return;

  for( WORD l = 0; l < 256; ++l )
  {
    int letterStart = 8*(l&0xe0) + (l&0x1f);

    for( BYTE r = 0; r < 8; ++r )
    {
      BYTE b = from[l + 256*r];
      for( BYTE i = 0; i < 8; ++i )
      {
        int index = 8*letterStart + 32*8*r + i;
        if( b & mask[i] )
          pg->pix[index] = black;
        else
          pg->pix[index] = white;
      }
    }
  }
  return;
}


class ROMFont : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(!hdr || hdr->length != 768 || hdr->noSex != 3) return 0;

      int sum = 0;
      for( int i = 0; i < 8; ++i ) sum += body[i];
      if(sum == 0) return 9; // check for 'space'

      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "Standart (ROM) font";
      font2image(im, body, 96);
      return im;
    }
} __ROMFont__;

class UDGFont : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 152 && hdr->noSex == 1) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "UDG font";
      font2image(im, body, 19);
      return im;
    }
} __UDGFont__;

class Font224 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 1792 && hdr->noSex == 7) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "224 characters font (32..255)";
      font2image(im, body, 224);
      return im;
    }
} __Font224__;

class Font256 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 2048 && hdr->noSex == 8) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      // detect font format using the fact that in
      // the most cases the first line is empty
      // I don't have better idea :(
      int n = 0;
      for( int i = 0; i < 2048; i += 8 ) if( body[i] == 0 ) ++n;

      Image* im = new Image();

      if(n >= 128)
      {
        font2image(im, body, 256);
        font2image4x8(im, body);
        im->format = "256 characters font";
      }
      else
      {
        font2image4x8(im, body);
        font2image(im, body, 256);
        im->format = "256 chars font";
      }
      return im;
    }
} __Font256__;

// 256 characters 4x8 font (2 chars in one block)
// used in STS 7.x by AlCo
class Font4x8 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 1024 && hdr->noSex == 4) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "256 characters 4x8 font";
      font2image(im, body, 128);
      return im;
    }
} __Font4x8__;

// 224 characters 6x7 font (packed)
// used in STS 7.x by AlCo

class Font6x7 : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 1008 && hdr->noSex == 4) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "224 characters 6x7 font";
      Page* pg = im->addPage(192, 42);

      if(pg)
      {
        for(size_t l = 0; l < 6; ++l)
          for(size_t p = 0; p < 7; ++p)
            for(size_t i = 0; i < 24; ++i)
            {
              BYTE bits = body[24*7*l + 24*p + i];

              for(size_t b = 0; b < 8; ++b)
              {
                size_t index = 6*24*8*p + 8*i + b + 24*8*l;
                if( bits & mask[b] )
                  pg->pix[index] = black;
                else
                  pg->pix[index] = white;
              }
            }
      }
      return im;
    }
} __Font6x7__;
}
