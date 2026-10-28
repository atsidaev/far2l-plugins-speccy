#ifndef common_hpp
#define common_hpp
#include "../image.hpp"

namespace theX
{

extern BGR black;
extern BGR white;
extern BGR palette[];
extern BYTE mask[];

bool scr(Image* im, const BYTE* data, bool secondStep = false, bool skipAttr = false);


Image* scr2image(const BYTE* scr, bool hasAttr = true);
template<size_t N>
bool checkSignature(const BYTE* data, const BYTE (&signature)[N])
{
  for(size_t i = 0; i < N; ++i)
    if(data[i] != signature[i]) return false;
  return true;
}

bool dehrust(const BYTE * source, size_t size, BYTE * destination, size_t capacity);
}

#endif
