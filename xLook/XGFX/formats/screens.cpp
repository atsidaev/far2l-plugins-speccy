#include "../image.hpp"
#include "../format.hpp"
#include "common.hpp"

namespace theX
{

class ColorScreen : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(size == 6912 && !hdr) {
        // check for exe signature
        if(body[0] != 'M' || body[1] != 'Z') return 9;
      }
      if(hdr && hdr->length == 6912 && hdr->noSex == 0x1b) {
        if(body[0] != 'M' || body[1] != 'Z') return 9;
      }
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = scr2image(body);
      im->format = "screen";
      return im;
    }
} __ColorScreen__;


class BWScreen : public Format
{
  public:
    int detect(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      if(size == 6144 && !hdr) {
        if(body[0] != 'M' || body[1] != 'Z') return 9;
      }
      if(hdr && hdr->length == 6144 && hdr->noSex == 24) {
        if(body[0] != 'M' || body[1] != 'Z') return 9;
      }
      return 0;
    }

    Image* convert(const HoHdr* hdr, const BYTE* body, size_t size)
    {
      Image* im = scr2image(body, false);
      im->format = "b&w screen";
      return im;
    }
} __BWScreen__;
}
