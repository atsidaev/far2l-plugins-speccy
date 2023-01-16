#ifndef udi_filer_hpp
#define udi_filer_hpp

#include <windows.h>

class FilerUDI
{
  public:
    FilerUDI(const char* fileName);
    ~FilerUDI();
    
    bool open (void);
    bool close(void);

    bool read (BYTE trk, BYTE sec, BYTE* buf);
    bool write(BYTE trk, BYTE sec, BYTE* buf);

    bool isProtected(void);
    bool protect    (bool on);
  private:
    HANDLE hostFile;
    char   fName[300];
    WORD   maxSec;
    bool   isChanged;
    DWORD* secs;
};

#endif
