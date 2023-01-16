#ifndef fdd_filer_hpp
#define fdd_filer_hpp

#include <windows.h>

class FilerFDD
{
  public:
    FilerFDD(const char* fileName);
    ~FilerFDD();
    
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
    DWORD* secs;
};

#endif
