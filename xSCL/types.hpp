#ifndef types_hpp
#define types_hpp

const int sectorSize = 0x100;

enum FileType { FMT_SCL, FMT_HOBETA, FMT_PLAIN };

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
  char  name[13];
  char  comment[100];
  BYTE  type;
  DWORD noSecsBefore;
  bool  deleted;
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

struct Options
{
  bool showExt;
  int  defaultPanelMode;
  int  defaultFormat;
  bool detectFormat;
  bool reread;
  char iniFilePath[300];
};

struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  unsigned int  Selected;
  unsigned int  Flags;
  unsigned char DefaultButton;
  char          *Data;
};

#endif
