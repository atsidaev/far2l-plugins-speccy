#include "../image.hpp"
#include "../format.hpp"
#include "common.hpp"

namespace theX
{

#pragma pack(push, 1)
struct Chunk
{
  BYTE line[4];
};

Chunk chunkSet[] =
{
  { 0x00, 0x00, 0x00, 0x00 },
  { 0x88, 0x00, 0x00, 0x00 },
  { 0x88, 0x00, 0x22, 0x00 },
  { 0x88, 0x00, 0xAA, 0x00 },
  { 0xAA, 0x00, 0xAA, 0x00 },
  { 0xAA, 0x44, 0xAA, 0x00 },
  { 0xAA, 0x44, 0xAA, 0x11 },
  { 0xAA, 0x44, 0xAA, 0x55 },
  { 0xAA, 0x55, 0xAA, 0x55 },
  { 0xAA, 0x55, 0xBB, 0x55 },
  { 0xEE, 0x55, 0xBB, 0x55 },
  { 0xFF, 0x55, 0xBB, 0x55 },
  { 0xFF, 0x55, 0xFF, 0x55 },
  { 0xFF, 0x77, 0xFF, 0x55 },
  { 0xFF, 0x77, 0xFF, 0xDD },
  { 0xFF, 0xFF, 0xFF, 0xFF }
};
#pragma pack(pop)

void chunk2image(Image* im, const BYTE* data)
{
  Page* pg = im->addPage();
  if(pg)
  {
    for(size_t y = 0; y < 48; ++y)
      for(size_t x = 0; x < 64; ++x)
      {
        Chunk ch = chunkSet[data[64 * y + x] & 0x0f];

        for(size_t l = 0; l < 4; ++l)
          for(size_t b = 0; b < 4; ++b)
          {
            BGR color = (ch.line[l] & mask[b]) ? white : black;
            pg->pix[256*4*y + 256*l + 4*x + b] = color;
          }
      }

    Page* pg = im->addPage( 256/4, 192/4 );
    if(pg)
      for(int i = 0; i < 3072; ++i)
        pg->pix[i].r = pg->pix[i].g = pg->pix[i].b = (data[i] << 4) | 0xf;
  }
}

// HardCore 4x4 chunk screen [0000.xxxx] or [nnnn.mmmm] where nnnn = mmmm
// to optimize for speed: replace loop with type+start check
// should be 'CH$'
class ChunkScreen : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(!hdr || hdr->length != 3072 || hdr->noSex != 12) return 0;

      bool ok = true;
      for(int i = 0; i < 3072; ++i)
        if(body[i] & 0xf0) { ok = false; break; }
      if(ok) return 9;

      for(int i = 0; i < 3072; ++i)
        if(((body[i] & 0xf0)>>4) != (body[i] & 0x0f)) return 0;

      return 9;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "Chunk screen pix/byte";
      chunk2image(im, body);
      return im;
    }
} __ChunkScreen__;

// HardCore 4x4 packed chunk screen
class PackedChunkScreen : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(hdr && hdr->length == 1536 && hdr->noSex == 6) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = new Image();
      im->format = "Chunk screen 2pix/byte";
      Page* pg = im->addPage();
      if(pg)
      {
        for(size_t y = 0; y < 48; ++y)
          for(size_t x = 0; x < 32; ++x)
          {
            Chunk ch1 = chunkSet[(body[32 * y + x]>>4) & 0x0f];

            for(size_t l = 0; l < 4; ++l)
              for(size_t b = 0; b < 4; ++b)
              {
                BGR color = (ch1.line[l] & mask[b]) ? white : black;
                pg->pix[256*4*y + 256*l + 8*x + b] = color;
              }

            Chunk ch2 = chunkSet[body[32 * y + x] & 0x0f];
            for(size_t l = 0; l < 4; ++l)
              for(size_t b = 0; b < 4; ++b)
              {
                BGR color = (ch2.line[l] & mask[b]) ? white : black;
                pg->pix[256*4*y + 256*l + 8*x+4 + b] = color;
              }
          }

        Page* pg = im->addPage( 256/4, 192/4 );
        if(pg)
        {
          for(int i = 0; i < 1536; ++i)
          {
            pg->pix[2*i].r = pg->pix[2*i].g = pg->pix[2*i].b = (body[i] & 0xf0) | 0xf;
            pg->pix[2*i+1].r = pg->pix[2*i+1].g = pg->pix[2*i+1].b = (body[i] << 4) | 0xf;
          }
        }
      }
      return im;
    }
} __PackedChunkScreen__;
}
