#ifndef __formatFactory_hpp__
#define __formatFactory_hpp__

#include <windows.h>
#include "image.hpp"

namespace theX
{

class Format;

class Factory
{
  private:
    static Factory* one;
    size_t length;
    Format* formats[50];

    Factory() { length = 0; }
  public:
    static Factory* get(void)
    {
      if(!one) one = new Factory();
      return one;
    }
    void add(Format* f) { if(length < 50) formats[length++] = f; }
    size_t noFormats(void) { return length; }

    Format* detect(const HoHdr* hdr, const BYTE* body, size_t size);
};

}

#endif
