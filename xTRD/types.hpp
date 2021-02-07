#ifndef types_hpp
#define types_hpp
#include <windows.h>

const int sectorSize = 0x100;

struct FileHdr
{
  char name[8];
  char type;
  WORD start;
  WORD size;
  BYTE noSecs;
  BYTE sec;
  BYTE trk;
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

struct SCLHdr
{
  char name[8];
  char type;
  WORD start;
  WORD size;
  BYTE noSecs;
};

struct ExtFileHdr
{
  char name   [13];
  char comment[100];
  BYTE type;
  bool skipHeader;
};

struct DiskHdr
{
 BYTE firstFreeSec;
 BYTE firstFreeTrk;
 BYTE type;
 BYTE noFiles;
 WORD noFreeSecs;
 BYTE reserved[13];
 BYTE noDelFiles;
 char title[11];
};

struct DSHdr
{
  WORD crc;
  char signature[6+3];
};

struct Options
{
  bool useDS;
  int  defaultPanelMode;
  bool showExt;
  bool autoMove;
  int  defaultFormat;
  bool detectFormat;

  char cmdLine1[300];
  char cmdLine2[300];
  char types1  [100];
  char types2  [100];
  bool fullScreen1;
  bool fullScreen2;
  char iniFilePath[300];
  
  bool reread;
};

struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  unsigned int  Selected;
  unsigned int  Flags;
  unsigned char DefaultButton;
  char         *Data;
};

enum ExitCode { OK, SKIP, CANCEL };

#endif
