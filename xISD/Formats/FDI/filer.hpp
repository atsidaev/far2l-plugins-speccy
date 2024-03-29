#ifndef fdi_filer_hpp
#define fdi_filer_hpp

#include <windows.h>

class Filer
{
  public:
    Filer(char* fileName);
    ~Filer();
    
    bool openFile (void);
    bool closeFile(void);

    bool read (WORD blockNum, BYTE* buf);
    bool write(WORD blockNum, BYTE* buf);
  private:
    HANDLE hostFile;
    char   fName[300];
    DWORD* blks;
};

#endif
