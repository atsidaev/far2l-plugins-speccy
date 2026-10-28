#ifndef types_hpp
#define types_hpp
#include <windows.h>

#pragma pack(1)

struct ZipedFileHdr
{
  char  name[8];
  char  type;
  WORD  start;
  WORD  size;
  BYTE  noSecs;

  WORD  packedSize;
  DWORD crc;
  BYTE  pMethod;
  BYTE  flags;
};

struct FileHdr
{
  char name[8];
  char type;
  WORD start;
  WORD size;
  BYTE noSecs;
};

struct ExtFileHdr
{
  char name[64];
  char  comment[100];
  BYTE  type;
  DWORD offset;
  bool  skipHeader;
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
