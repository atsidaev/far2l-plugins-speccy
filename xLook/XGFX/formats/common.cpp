#include "../../../shared/hrust.hpp"
#include "../image.hpp"

namespace theX
{

extern "C" int __cdecl _purecall(void) { return 0; }

BYTE mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
BGR  black  = { 0, 0, 0 };
BGR  white  = { 0xff, 0xff, 0xff };

#define N0    8
#define N1    191
#define B0    32
#define B1    255

BGR palette[] =
{
    {0,0,0}, {N1,N0,N0}, {N0,N0,N1}, {N1,N0,N1}, {N0,N1,N0}, {N1,N1,N0}, {N0,N1,N1}, {N1,N1,N1},
    {0,0,0}, {B1,B0,B0}, {B0,B0,B1}, {B1,B0,B1}, {B0,B1,B0}, {B1,B1,B0}, {B0,B1,B1}, {B1,B1,B1}
};


bool scr(Image* im, const BYTE* data, bool secondStep = false, bool skipAttr = false)
{
  Page* pg = im->addPage();

  bool flash = false;
  if(pg)
  {
    if(secondStep) im->animate = true;

    for(size_t y = 0; y < 192; ++y)
    {
      const BYTE* pixelLine = data + 32 * ((y & 0xc0) | ((y << 3) & 0x38) | ((y >> 3) & 0x07));
      const BYTE* attr      = data + 6144 + ((y & 0xf8) << 2);

      for(size_t x = 0; x < 32; ++x)
      {
        BYTE paper = (attr[x] >> 3  ) & 0x0f;
        BYTE ink   = (attr[x] & 0x07) | (paper & 0x08);
        BYTE pix   = pixelLine[x];

        if(skipAttr) { ink = 0; paper = 7; }

        if((attr[x] & 0x80) && paper != ink) flash = true;

        for(size_t bit = 0; bit < 8; ++bit)
        {
          size_t i  = 256*y + 8*x + bit;
          size_t ci = (pix & mask[bit]) ? ink : paper;

          if(secondStep && (attr[x] & 0x80))
              ci = (pix & mask[bit]) ? paper : ink;

          pg->pix[i] = palette[ci];
        }
      }
    }
  }
  return flash;
}

Image* scr2image(const BYTE* data, bool hasAttr)
{
  Image* im = new Image();

  BYTE buf[6912];
  const BYTE* p = data;

  if(!hasAttr)
  {
    for(int i = 0; i < 6144; ++i) buf[i] = data[i];
    for(int i = 6144; i < 6912; ++i) buf[i] = 7;
    p = buf;
  }

  if(scr(im, p))
    scr(im, p, true);
  else
  {
    if(hasAttr)
      scr(im, p, false, true);
    else
    {
      // second page: screen in "sprite" format
      Page* pg = im->addPage();
      if(pg)
      {
        for(size_t y = 0; y < 192; ++y)
        {
          const BYTE* pixelLine = data + 32 * y;
          for(size_t x = 0; x < 32; ++x)
          {
            BYTE pix   = pixelLine[x];
            for(size_t bit = 0; bit < 8; ++bit)
            {
              size_t i  = 256*y + 8*x + bit;
              size_t ci = (pix & mask[bit]) ? 0 : 7;

              pg->pix[i] = palette[ci];
            }
          }
        }
      }
    }
  }
  return im;
}

bool dehrust(const BYTE * source, size_t size, BYTE * destination, size_t capacity) {
  size_t written = 0;
  return speccy::hrust1(source, size, destination, capacity, written) && written == capacity;
}
}
