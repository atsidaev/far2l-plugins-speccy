#include "../image.hpp"
#include "../format.hpp"
#include "common.hpp"

namespace theX
{

class SpritesGenerator50 : public Format {
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size) {
      if(hdr && hdr->type == 's' && hdr->start == 38138) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size) {
      Image* im = new Image;
      im->format = "Sprites Generator 5.0 sprites";

      BYTE n = body[0];
      const BYTE* data = body + 5 + 3 * n;

      for(size_t i = 0; i < n; ++i) {
        BYTE w = body[i * 3 + 1];
        BYTE h = body[i * 3 + 2];

        BYTE type = body[i * 3 + 3];

        Page* pg = im->addPage(8 * w, 8 * h);
        if(!pg) break;


        for(size_t y = 0; y < 8 * h; ++y) {
          for(size_t x = 0; x < w; ++x) {

            size_t ink = 0;
            size_t paper = 7;
            size_t offset = 0;

            if(type & 0x01) { // with attr
              BYTE attr = data[w * (y/8) + x];

              paper = (attr >> 3  ) & 0x0f;
              ink = (attr & 0x07) | (paper & 0x08);

              offset = w * h;
            }

            size_t step = 1;
            if(type & 0x10) step = 2; // with mask

            BYTE dots = data[offset + step * (w * y + x)];

            for(size_t bit = 0; bit < 8; ++bit) {
              size_t ci = (dots & mask[bit]) ? ink : paper;

              size_t index = 8 * (w * y  + x) + bit;

              pg->pix[index] = palette[ci];
            }
          }
        }

        data += w * h * type;
      }
      return im;
    }
} __SpritesGenerator50__;


class SPwRITE_MAKER : public Format {
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size) {
      BYTE signature[] = { 'S','P','W','R',' ','v','4',' ','F','I','L','E', 0 };
      if(checkSignature(body, signature)) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size) {
      Image* im = new Image;
      im->format = "SPwRITE MAKER v4 sprites";

      BYTE w = body[13] + 1;
      BYTE h = body[14] + 1;

      BYTE n = body[15] + 1;

      const BYTE* data = body + 32;

      for(size_t i = 0; i < n; ++i) {

        Page* pg = im->addPage(8 * w, h);
        if(!pg) break;


        for(size_t y = 0; y < h; ++y) {
          for(size_t x = 0; x < w; ++x) {

            size_t ink = 7;
            size_t paper = 0;

            BYTE dots = data[w * y + x];

            for(size_t bit = 0; bit < 8; ++bit) {
              size_t ci = (dots & mask[bit]) ? ink : paper;

              size_t index = 8 * (w * y  + x) + bit;

              pg->pix[index] = palette[ci];
            }
          }
        }

        data += 2 * w * h; // skip sprite & mask
      }
      return im;
    }
} __SPwRITE_MAKER__;

class SpriteMaker : public Format {
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size) {
      if(!hdr || hdr->ext[2] < 32 || hdr->type != 'C') return 0;

      BYTE dimention = hdr->ext[1];
      BYTE w = dimention & 0x0f;
      BYTE h = dimention >> 4;

      if(w == 0 || h == 0) return 0;

      BYTE chipTrackerSignature[] = {'C', 'H', 'I', 'P', 'v'};
      if(checkSignature(body, chipTrackerSignature)) return 0;

      if(hdr->length % (8 * w * h) == 0) return 9;

      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size) {
      Image* im = new Image;
      im->format = "Sprite Maker 1.6x sprites";
      im->animate = true;

      BYTE dimention = hdr->ext[1];
      BYTE w = dimention & 0x0f;
      BYTE h = dimention >> 4;

      BYTE n = hdr->length / (8 * w * h);

      const BYTE* data = body;

      for(size_t i = 0; i < n; ++i) {

        Page* pg = im->addPage(8 * w, 8 * h);
        if(!pg) break;


        for(size_t y = 0; y < 8 * h; ++y) {
          for(size_t x = 0; x < w; ++x) {

            size_t ink = 7;
            size_t paper = 0;

            BYTE dots = data[w * y + x];

            for(size_t bit = 0; bit < 8; ++bit) {
              size_t ci = (dots & mask[bit]) ? ink : paper;

              size_t index = 8 * (w * y  + x) + bit;

              pg->pix[index] = palette[ci];
            }
          }
        }

        data += 8 * w * h;
      }
      return im;
    }
} __SpriteMaker__;


#pragma pack(push, 1)
struct CharCell {
  BYTE pix[8];
  BYTE color;
};
#pragma pack(pop)


const BYTE* unpack_cell_1_3(BYTE* pix, const BYTE* ptr, bool method1 = true) {
  BYTE b = 0;
  BYTE xb = 0;
  BYTE flags = *ptr++;

  for(size_t i = 0; i < 8; ++i) {
    if(flags & mask[7-i]) xb = *ptr++;

    b ^= xb;
    if(method1)
      pix[i] = b;
    else
      pix[i] ^= b;
  }
  return ptr;
}

const BYTE* unpack_cell_2_4(BYTE* pix, const BYTE* ptr, bool method2 = true) {
  BYTE b = 0;
  BYTE flags = *ptr++;

  for(size_t i = 0; i < 8; ++i) {
    if(flags & mask[7-i]) b ^= *ptr++;

    if(method2)
      pix[i] = b;
    else
      pix[i] ^= b;
  }
  return ptr;
}

class ANI : public Format {
  private:
    void frame2pix(const CharCell* cells, BGR* pix, size_t w, size_t h)
    {
      WORD H = h/8 + (h%8 ? 1 : 0);

      for(size_t y = 0; y < H; ++y)
      {
        size_t maxline = 8;
        // last row can be narrower
        if(y == H - 1 && h % 8) maxline = h % 8;

        for(size_t x = 0; x < w; ++x) {

          BYTE color = cells[w * y + x].color;
          BYTE paper = (color >> 3  ) & 0x0f;
          BYTE ink   = (color & 0x07) | (paper & 0x08);

          for(size_t i = 0; i < maxline; ++i)
          {
            BYTE dots = cells[w * y + x].pix[i];

            for(size_t bit = 0; bit < 8; ++bit) {
              size_t ci = (dots & mask[bit]) ? ink : paper;

              size_t index = 8 * (w * (8 * y +i)  + x) + bit;

              pix[index] = palette[ci];
            }
          }
        }
      }
    }

  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size) {
      BYTE aniSignature[] = "GIF animation";

      if(checkSignature(body, aniSignature)) {
        return 9;
      }
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size) {
      Image* im = new Image;
      im->format = "ani-file v.5-";
      im->animate = true;

      BYTE w = body[14];
      BYTE h = body[15];

      WORD H = h/8 + (h%8 ? 1 : 0);

      const BYTE* ptr = body + 16;

      CharCell cells[32*24];

      for(size_t i = 0; i < 32*24; ++i) {
        cells[i].color = 7;
        for(size_t l = 0; l < 8; ++l) cells[i].pix[l] = 0;
      }

      while(ptr - body < hdr->length) {
        BYTE delay = *ptr++;
        BYTE packType = *ptr++;

        bool hasColor = !!(packType & 0x80);
        bool packedColors = !!(packType & 0x40);

        packType &= 0x3f;

        if(packType > 8 || delay == 255) break;

        Page* pg = im->addPage(8 * w, h);
        if(!pg) break;

        if(packType == 0)
        {
          for(size_t y = 0; y < H; ++y) {
            size_t maxline = 8;
            // last row can be narrower
            if(y == H - 1) maxline = h % 8;

            for(size_t i = 0; i < maxline; ++i)
            {
              for(size_t x = 0; x < w; ++x)
                cells[w * y + x].pix[i] = *ptr++;
            }

            if(hasColor) {
              for(size_t x = 0; x < w; ++x) {
                cells[w * y + x].color = *ptr++;
              }
            }
          }
        }
        else
        {
          BYTE second_level_flag = 0;
          BYTE color_flag = 0;
          int idx = 0;

          for(size_t y = 0; y < H; ++y)
          {
            for(size_t x = 0; x < w; ++x)
            {
              CharCell& cell = cells[w * y + x];

              if(idx % 8 == 0)
              {
                if(packType > 4) second_level_flag = *ptr++;
                if(hasColor && packedColors) color_flag = *ptr++;

                idx = 0;
              }

              switch(packType)
              {
                case 1:
                  ptr = unpack_cell_1_3(cell.pix, ptr, true);
                  break;
                case 2:
                  ptr = unpack_cell_2_4(cell.pix, ptr, true);
                  break;
                case 3:
                  ptr = unpack_cell_1_3(cell.pix, ptr, false);
                  break;
                case 4:
                  ptr = unpack_cell_2_4(cell.pix, ptr, false);
                  break;

                case 5:
                {
                  if(second_level_flag & mask[7-idx])
                    ptr = unpack_cell_1_3(cell.pix, ptr, true);
                  else
                    for(size_t i = 0; i < 8; ++i) cell.pix[i] = 0;
                  break;
                }
                case 6:
                {
                  if(second_level_flag & mask[7-idx])
                    ptr = unpack_cell_2_4(cell.pix, ptr, true);
                  else
                    for(size_t i = 0; i < 8; ++i) cell.pix[i] = 0;
                  break;
                }
                case 7:
                {
                  if(second_level_flag & mask[7-idx])
                    ptr = unpack_cell_1_3(cell.pix, ptr, false);
                  break;
                }
                case 8:
                {
                  if(second_level_flag & mask[7-idx])
                    ptr = unpack_cell_2_4(cell.pix, ptr, false);
                  break;
                }
              }

              if(hasColor)
              {
                if(!packedColors)
                  cell.color = *ptr++;
                else
                {
                  if(color_flag & mask[7-idx]) cell.color = *ptr++;
                }
              }

              ++idx;
            }
          }
        }
        frame2pix(cells, pg->pix, w, h);
      }
      return im;
    }
} __ANI__;

class SpriteEditor11 : public Format {
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size) {
      if(hdr && hdr->type == 'G' && hdr->start == 32765) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size) {
      Image* im = new Image;
      im->format = "Sprite Editor 1.1 sprites";
      im->animate = true;

      BYTE w = body[0] & 0x0f;
      BYTE h = body[1];
      BYTE n = body[2];

      size_t offset = 3;

      for(size_t i = 0; i < n; ++i) {
        Page* pg = im->addPage(8 * w, h);
        if(!pg) break;


        for(size_t y = 0; y < h; ++y) {
          for(size_t x = 0; x < w; ++x) {
            size_t ink = 7;
            size_t paper = 0;

            BYTE dots = body[offset + (w * y + x)];

            for(size_t bit = 0; bit < 8; ++bit) {
              size_t ci = (dots & mask[bit]) ? ink : paper;

              size_t index = 8 * (w * y  + x) + bit;

              pg->pix[index] = palette[ci];
            }
          }
        }

        offset += w * h;
      }
      return im;
    }
} __SpriteEditor11__;

class SpriteEditor21 : public Format {
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size) {
      if(hdr && hdr->type == 'G' && hdr->start == 36861) return 9;
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size) {
      Image* im = new Image;
      im->format = "Sprite Editor 2.1 sprites";
      im->animate = true;

      BYTE w = body[0] & 0x0f;
      BYTE h = body[1];
      BYTE n = body[2];

      bool withMask = body[0] & 0x80;
      bool withColor = body[0] & 0x40;

      size_t offset = 3;
      if(withMask) offset += w * h;

      for(size_t i = 0; i < n; ++i) {
        Page* pg = im->addPage(8 * w, h);
        if(!pg) break;


        for(size_t y = 0; y < h; ++y) {
          for(size_t x = 0; x < w; ++x) {
            size_t ink = 7;
            size_t paper = 0;

            if(withColor) {
              BYTE attr = body[offset + w * h + w * (y/8) + x];

              paper = (attr >> 3  ) & 0x0f;
              ink = (attr & 0x07) | (paper & 0x08);
            }

            BYTE dots = body[offset + (w * y + x)];

            for(size_t bit = 0; bit < 8; ++bit) {
              size_t ci = (dots & mask[bit]) ? ink : paper;

              size_t index = 8 * (w * y  + x) + bit;

              pg->pix[index] = palette[ci];
            }
          }
        }
        offset += w * h;
        if(withMask) offset += w * h;
      }
      return im;
    }
} __SpriteEditor21__;
}
