#ifndef __formats_hpp__
#define __formats_hpp__

#include <windows.h>
#include "formatFactory.hpp"

namespace theX
{

class Format
{
  public:
    Format(void) { Factory::get()->add(this); }
    virtual int    detect (const HoHdr* hdr, const BYTE* body, size_t size) = 0;
    virtual Image* convert(const HoHdr* hdr, const BYTE* body, size_t size) = 0;
};

}

#endif
