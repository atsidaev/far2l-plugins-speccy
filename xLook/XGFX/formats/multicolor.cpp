#include "../image.hpp"
#include "../format.hpp"
#include "common.hpp"

namespace theX
{

bool mc128x96(Image* im, const BYTE* body, bool secondStep = false)
{
  Page* pg = im->addPage(16*8, 12*8);
  bool flash = false;
  if(pg)
  {
    if(secondStep) im->animate = true;

    for( int y = 0; y < 12*8; ++y )
    {
      const BYTE* pixelLine = body + 16 * y;
      const BYTE* colorLine = body + 0x600 + 16 * y;
      for( int x = 0; x < 16; ++x )
      {
        BYTE paper = (colorLine[x] >> 3  ) & 0x0f;
        BYTE ink   = (colorLine[x] & 0x07) | (paper & 0x08);
        BYTE pix   = pixelLine[x];
        if((colorLine[x] & 0x80) && paper != ink) flash = true;

        for( int i = 0; i < 8; ++i )
        {
          int index = 16*8*y+8*x+i;
          int ci    = (pix & mask[i]) ? ink : paper;
          if(secondStep && (colorLine[x] & 0x80))
            ci = (pix & mask[i]) ? paper : ink;
          pg->pix[index] = palette[ci];
        }
      }
    }
  }
  return flash;
}

class MultiColor128x96 : public Format
{
  public:
    int detect (const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 3072 && hdr->noSex == 12) return 5;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;
      im->format = "Multicolor 128x96 [MS 1.08]";

      if( mc128x96(im, body) ) mc128x96(im, body, true);
      return im;
    }
} __MultiColor128x96__;

bool mc192x192(Image* im, const BYTE* body, bool secondStep = false)
{
  Page* pg = im->addPage(192, 192);
  bool flash = false;

  if(pg)
  {
    if(secondStep) im->animate = true;

    for( int y = 0; y < 192; ++y )
    {
      const BYTE* pixelLine = body + 24 * y + 0x100;
      const BYTE* colorLine = body + 0x1200 + 24 * (y/2) + 0x100;
      for( int x = 0; x < 24; ++x )
      {
        BYTE paper = (colorLine[x] >> 3  ) & 0x0f;
        BYTE ink   = (colorLine[x] & 0x07) | (paper & 0x08);
        BYTE pix   = pixelLine[x];

        if((colorLine[x] & 0x80) && paper != ink) flash = true;

        for( int i = 0; i < 8; ++i )
        {
          int index = 192*y+8*x+i;
          int ci    = (pix & mask[i]) ? ink : paper;
          if(secondStep && (colorLine[x] & 0x80))
            ci = (pix & mask[i]) ? paper : ink;
          pg->pix[index] = palette[ci];
        }
      }
    }
  }
  return flash;
}

class MultiColor192x192 : public Format
{
  public:
    int detect (const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(!hdr || hdr->length != 7168 || hdr->noSex != 28) return 0;

      BYTE signature[] = { 'M', 'E', 'G', 'A', '-', 'S', 'C', 'R', 'E', 'E', 'N', 0x20, 'F', 'I', 'L', 'E' };
      if(checkSignature(body, signature)) return 9;

      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;
      im->format = "Multicolor 192x192 2x8 [MS 2.5]";
      if( mc192x192(im, body) ) mc192x192(im, body, true);

      return im;
    }
} __MultiColor192x192__;

void mc(const BYTE* data, BGR* p)
{
  for( int y = 0; y < 192; ++y )
  {
    const BYTE* pixelLine = data + 32 * y;
    const BYTE* colorLine = data + 6144 + 32 * y;
    for( int x = 0; x < 32; ++x )
    {
      BYTE paper = (colorLine[x] >> 3  ) & 0x0f;
      BYTE ink   = (colorLine[x] & 0x07) | (paper & 0x08);
      BYTE pix   = pixelLine[x];

      for( int i = 0; i < 8; ++i )
      {
        int index = 256*y+8*x+i;
        int ci    = (pix & mask[i]) ? ink : paper;
        p[index] = palette[ci];
      }
    }
  }
}

class MultiColor : public Format
{
  public:
    int detect (const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 2*6144 && hdr->noSex == 2*24) return 5;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;
      im->format = "MultiColor";
      Page* pg = im->addPage();
      if(pg) mc(body, pg->pix);
      return im;
    }
} __MultiColor__;

class DoubleMultiColor : public Format
{
  public:
    int detect (const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 4*6144 && hdr->noSex == 4*24) return 5;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;
      im->format = "Doublescreen MultiColor";
      BGR pix[256*192];
      mc(body, pix);

      Page* pg = im->addPage();
      if(pg)
      {
        mc(body+2*6144, pg->pix);
        for( int i = 0; i < 256*192; ++i )
        {
          pg->pix[i].r = (pg->pix[i].r + pix[i].r)/2;
          pg->pix[i].g = (pg->pix[i].g + pix[i].g)/2;
          pg->pix[i].b = (pg->pix[i].b + pix[i].b)/2;
        }
      }
      return im;
    }
} __DoubleMultiColor__;
}
