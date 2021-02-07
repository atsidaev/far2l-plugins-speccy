#ifndef filer_hpp
#define filer_hpp

class Filer
{
  public:
    Filer(const char* fileName);
    ~Filer();
    
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
