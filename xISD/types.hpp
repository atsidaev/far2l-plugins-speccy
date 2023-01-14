#ifndef types_hpp
#define types_hpp

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

const int blockSize = 0x100;

struct InitDialogItem
{
  u8    Type;
  u8    X1,Y1,X2,Y2;
  u8    Focus;
  u32   Selected;
  u32   Flags;
  u8    DefaultButton;
  char* Data;
};

enum Action { DO_ALL, SKIP_ALL, ASK_USER };

struct Options
{
  int  defaultPanelMode;
  char columnTypes [100];
  char columnWidths[100];
  char columnTitles[100];
  bool reread;
};

#endif
