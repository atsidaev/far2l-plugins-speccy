#ifndef types_hpp
#define types_hpp
#include <windows.h>

#pragma pack(1)

struct BlockHdr
{
  char signature[5];
  BYTE flags;
  WORD size;
  WORD pSize;
  BYTE extInfoSize;
};

struct FileHdr
{
  char name[8];
  char type;
  WORD start;
  WORD size;
  BYTE noSecs;
};

struct ExtInfo
{
  WORD    pCRC;
  WORD    CRC;
  FileHdr file;
};

struct ExtFileHdr
{
  char name[64];
  char  comment[100];
  BYTE  type;
  bool  skipHeader;
};

struct PackInfo
{
  WORD  packedSize;
  DWORD offset;
};

struct HoHdr
{
  char name[8];
  char type;
  WORD start;
  WORD size;
  BYTE reserved;
  BYTE noSecs;
  WORD checkSum;
};

#pragma pack()

struct Options
{
  bool showExt;
  int  defaultPanelMode;
  int  defaultFormat;
  bool detectFormat;
  char iniFilePath[512];
};

struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  DWORD_PTR     Selected;
  unsigned int  Flags;
  unsigned char DefaultButton;
  const char   *Data;
};


#endif
