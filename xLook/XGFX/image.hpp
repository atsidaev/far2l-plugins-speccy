#ifndef image_hpp
#define image_hpp
#include <windows.h>

namespace theX
{

typedef BYTE byte;


#pragma pack(push, 1)
struct HoHdr
{
  char name[8];
  union
  {
    char ext[3];
    struct
    {
      char type;
      WORD start;
    };
  };
  WORD length;
  BYTE reserved;
  BYTE noSex;
  WORD checkSum;
};

struct BGR
{
  BYTE b;
  BYTE g;
  BYTE r;
};

#pragma pack(pop)

class Page
{
  public:
    size_t w;
    size_t h;
    BGR*   pix;

    Page(size_t width, size_t height)
    {
      w   = width;
      h   = height;
      pix = new BGR[w*h];
      memset(pix, 0, w*h*sizeof(BGR));
    }
    ~Page() { delete[] pix; }
};

class Image
{
  public:
    size_t noPages;
    bool   animate;
    Page*  pages[256];
    const char * format;
    Image(void) { noPages = 0; animate = false; format = 0;}
    ~Image() {for(size_t i = 0; i < noPages; ++i ) delete pages[i];}

    Page* addPage(size_t width = 256, size_t height = 192)
    {
      if(noPages < 256)
      {
        Page* p = new Page(width, height);
        if(p) pages[noPages++] = p;
        return p;
      }
      return 0;
    }

    Page* getPage(size_t n)
    {
      if(n >= noPages) return 0;
      return pages[n];
    }
};
}

#endif
