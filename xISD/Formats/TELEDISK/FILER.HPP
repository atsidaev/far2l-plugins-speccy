#ifndef filer_hpp
#define filer_hpp

class Filer
{
  public:
    Filer(char* fileName);
    ~Filer();
    
    bool openFile (void);
    bool closeFile(void);

    bool reload(void);

    bool read(WORD blockNum, BYTE* buf);
  private:
    HANDLE hostFile;
    char   fName[300];
    BYTE*  imageBuf;
    bool*  masks;
    WORD   noBlocks;
    BYTE   sectorSize;
};

#endif