#include "formatFactory.hpp"
#include "image.hpp"
#include "format.hpp"

namespace theX
{

Factory* Factory::one = 0;

Format* Factory::detect(const HoHdr* hdr, const BYTE* body, size_t size)
{
  Format* fmt = 0;
  int best = 0;

  for(size_t i = 0; i < length; ++i)
  {
    int res = formats[i]->detect(hdr, body, size);
    if(res == 0) continue;
    if(res == 9) return formats[i];
    if(res > best) { best = res; fmt = formats[i]; }
  }
  return fmt;
}
}
