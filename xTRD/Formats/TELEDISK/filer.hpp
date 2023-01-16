#ifndef td0_filer_hpp
#define td0_filer_hpp

#include <windows.h>

class FilerTD
{
  public:
    FilerTD(const char* fileName);
    ~FilerTD();
    
    bool open (void);
    bool close(void);

    bool reload(void);

    bool read(BYTE trk, BYTE sec, BYTE* buf);
  private:
    HANDLE hostFile;
    char   fName[300];
    BYTE*  imageBuf;
    WORD   masks[166];
};

#endif
