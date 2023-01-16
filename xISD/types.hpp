#ifndef types_hpp
#define types_hpp

#include <stdint.h>
#include <windows.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

const int blockSize = 0x100;

struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  DWORD_PTR Selected;
  unsigned int Flags;
  unsigned char DefaultButton;
  const char *Data;
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
