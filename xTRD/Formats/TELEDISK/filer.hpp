#ifndef filer_hpp
#define filer_hpp

class Filer
{
  public:
    Filer(const char* fileName);
    ~Filer();
    
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
