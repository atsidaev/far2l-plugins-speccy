#include <windows.h>
#include "common.hpp"
#include "../format.hpp"
#include "../image.hpp"

namespace theX
{

#pragma pack(push, 1)
typedef struct
{
  BYTE signature[3];
  BYTE noSprites;
  WORD dataSize;
  WORD reserve;
} BITFileHeader;

typedef struct
{
  BYTE width;
  BYTE height;
  WORD offsetWithFlag;
} BITSpriteHeader;
#pragma pack(pop)

class QHTS : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && body[0] == 'B' && body[1] == 'I' && body[2] == 'T') return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      BITFileHeader*   bitHdr = (BITFileHeader*)body;
      BITSpriteHeader* sprHdr = (BITSpriteHeader*)(body+sizeof(BITFileHeader));

      Image* im = new Image();
      im->format = "QHTS";

      for(size_t i = 0; i < bitHdr->noSprites; ++i)
      {
        bool withAttr = (sprHdr[i].offsetWithFlag & 0x8000) != 0;
        const BYTE* p = body + (sprHdr[i].offsetWithFlag & 0x7fff);

        BYTE w = sprHdr[i].width;
        BYTE h = sprHdr[i].height;

        Page* pg = im->addPage(8*w,8*h);
        if(!pg)
        {
          delete im;
          return 0;
        }

        BYTE buf[8];

        for(size_t y = 0; y < h; ++y)
          for(size_t x = 0; x < w; ++x)
          {
            BYTE bMask = *p++;
            BYTE diff  = 0;
            BYTE data  = 0;
            for(size_t b = 0; b < 8; ++b)
            {
              if(bMask & mask[7-b]) diff = *p++;
              data ^= diff;
              buf[b] = data;
            }

            BYTE attr = 0x07;
            if(withAttr) attr = *p++;
            BYTE paper = (attr >> 3) & 0x0f;
            BYTE ink   = (attr & 0x07) | (paper & 0x08);

            for(size_t l = 0; l < 8; ++l)
              for(size_t b = 0; b < 8; ++b)
              {
                size_t index = (8*y+l)*8*w + 8*x + b;
                size_t ci = (buf[l] & mask[b]) ? ink : paper;
                pg->pix[index] = palette[ci];
              }
          }
      }
      return im;
    }
} __QHTS__;
}
