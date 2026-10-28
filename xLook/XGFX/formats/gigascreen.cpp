#include "../image.hpp"
#include "../format.hpp"
#include "common.hpp"

namespace theX
{

Image* gigaScreen2image(const BYTE* data)
{
  Image* im = new Image;
  Page* pg = im->addPage();

  if(pg)
  {
    for(size_t y = 0; y < 192; ++y)
    {
      const BYTE* pixelLine = data + 32 * ((y & 0xc0) | ((y << 3) & 0x38) | ((y >> 3) & 0x07));
      const BYTE* attr      = data + 6144 + ((y & 0xf8) << 2);

      for(size_t x = 0; x < 32; ++x)
      {
        BYTE paper1 = (attr[x] >> 3  ) & 0x0f;
        BYTE ink1   = (attr[x] & 0x07) | (paper1 & 0x08);
        BYTE pix1   = pixelLine[x];

        BYTE paper2 = (attr[6912+x] >> 3  ) & 0x0f;
        BYTE ink2   = (attr[6912+x] & 0x07) | (paper2 & 0x08);
        BYTE pix2   = pixelLine[6912+x];

        for(size_t bit = 0; bit < 8; ++bit)
        {
          size_t i  = 256*y + 8*x + bit;
          size_t ci1 = (pix1 & mask[bit]) ? ink1 : paper1;
          size_t ci2 = (pix2 & mask[bit]) ? ink2 : paper2;

          pg->pix[i].r = (palette[ci1].r + palette[ci2].r)/2;
          pg->pix[i].g = (palette[ci1].g + palette[ci2].g)/2;
          pg->pix[i].b = (palette[ci1].b + palette[ci2].b)/2;
        }
      }
    }
  }

  scr(im, data);
  scr(im, data + 6912);

  return im;
}

class GigaScreen : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(size == 2*6912 && !hdr) {
        // check for exe signature
        if(body[0] != 'M' || body[1] != 'Z') return 9;
      }
      if(hdr && hdr->length == 2*6912 && hdr->noSex == 2*0x1b) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = gigaScreen2image(body);
      if(im) im->format = "GigaScreen";
      return im;
    }
} __GigaScreen__;

class HrustedGigaScreen : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(!hdr || size < 0x109) return 0;

      WORD data_size   = *(WORD*)(body+0x105);
      WORD packed_size = *(WORD*)(body+0x107);

      if(body[0x103] == 'H' && body[0x104] == 'R' &&
         data_size == 2*6912 &&
         packed_size < data_size && packed_size < size) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      BYTE buf[6912*2];
      if(!dehrust(body, size, buf, sizeof(buf)))
        return nullptr;

      Image* im = gigaScreen2image(buf);
      if(im) im->format = "GigaScreen (hrusted)";
      return im;
    }
} __HrustedGigaScreen__;
}
