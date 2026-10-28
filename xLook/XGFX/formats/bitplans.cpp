#include "../image.hpp"
#include "../format.hpp"
#include "common.hpp"

namespace theX
{

void brg2page(Image* im, const BYTE* data)
{
  Page* pg = im->addPage();

  if(pg)
  {
    for(size_t i = 0; i < 256*192; ++i) pg->pix[i] = black;

    for( int y = 0; y < 192; ++y )
    {
      const BYTE* pixelLine = data + 32 * ((y & 0xc0) | ((y << 3) & 0x38) | ((y >> 3) & 0x07));

      for( int x = 0; x < 32; ++x )
      {
        BYTE b = pixelLine[x];
        BYTE r = pixelLine[x+6144];
        BYTE g = pixelLine[x+2*6144];

        for( int i = 0; i < 8; ++i )
        {
          int index = 256*y+8*x+i;
          if( r & mask[i] ) pg->pix[index].r = 0xff;
          if( g & mask[i] ) pg->pix[index].g = 0xff;
          if( b & mask[i] ) pg->pix[index].b = 0xff;
        }
      }
    }
  }
}

void rgb2page(Image* im, const BYTE* data, bool sprite = false)
{
  Page* pg = im->addPage();

  if(pg)
  {
    for(size_t i = 0; i < 256*192; ++i) pg->pix[i] = black;

    for( int y = 0; y < 192; ++y )
    {
      const BYTE* pixelLine = 0;
      if(sprite)
        pixelLine = data + 32 * y;
      else
        pixelLine = data + 32 * ((y & 0xc0) | ((y << 3) & 0x38) | ((y >> 3) & 0x07));
      for( int x = 0; x < 32; ++x )
      {
        BYTE r = pixelLine[x];
        BYTE g = pixelLine[x+6144];
        BYTE b = pixelLine[x+2*6144];

        for( int i = 0; i < 8; ++i )
        {
          int index = 256*y+8*x+i;
          if( r & mask[i] ) pg->pix[index].r = 0xff;
          if( g & mask[i] ) pg->pix[index].g = 0xff;
          if( b & mask[i] ) pg->pix[index].b = 0xff;
        }
      }
    }
  }
}

class RGBScreens : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(size == 3*6144 && !hdr) {
        // check for exe signature
        if(body[0] != 'M' || body[1] != 'Z') return 9;
      }
      if(hdr && hdr->length == 3*6144 && hdr->noSex == 3*24) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;

      if(hdr && (hdr->type == 'b' || hdr->type == '3'))
      {
        im->format = "BRG screens";
        brg2page(im, body);
      }
      else
      {
        im->format = "RGB sprites/screens";
        rgb2page(im, body);
        rgb2page(im, body, true);

      }
      return im;
    }
} __RGBScreens__;


class HrustedRGBScreens : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(size <= 0x108) return 0;
      WORD data_size   = *(WORD*)(body+0x105);
      WORD packed_size = *(WORD*)(body+0x107);

      if(body[0x103] == 'H' && body[0x104] == 'R' &&
        data_size == 6144*3 &&
        packed_size < data_size && packed_size < size ) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      BYTE buf[6144*3];
      if(!dehrust(body, size, buf, sizeof(buf)))
        return nullptr;

      Image* im = new Image;
      im->format = "RGB sprites/screen (hrusted)";

      rgb2page(im, buf);
      rgb2page(im, buf, true);
      return im;
    }
} __HrustedRGBScreens__;

// MST (256x128 BRG sprite bitplanes)
class MST : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 32*128*3 && hdr->noSex == 48 && hdr->type == '+') return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image;
      im->format = "BRG sprites 256x128";

      Page* pg = im->addPage(256, 128);
      if(pg)
      {
        for( int y = 0; y < 128; ++y )
          for( int x = 0; x < 32; ++x )
          {
            byte b = body[32*y+x];
            byte r = body[32*y+x + 4096];
            byte g = body[32*y+x + 2*4096];
            for( int i = 0; i < 8; ++i )
            {
              int index = 256*y+8*x+i;
              pg->pix[index].r = r & mask[i] ? 0xff : 0;
              pg->pix[index].g = g & mask[i] ? 0xff : 0;
              pg->pix[index].b = b & mask[i] ? 0xff : 0;
            }
          }
      }
      return im;
    }
} __MST__;
}
