#ifndef fdi_filer_hpp
#define fdi_filer_hpp

#include <windows.h>

class FilerFDI
{
  public:
    FilerFDI(const char* fileName);
    ~FilerFDI();
    
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
    BYTE   writeProtection;
    DWORD* secs;
};

#endif
