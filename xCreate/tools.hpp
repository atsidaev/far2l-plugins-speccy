#ifndef utils_hpp
#define utils_hpp

#include <far2sdk/farplug-mb.h>
using namespace oldfar;
#include "trdos.hpp"

struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  DWORD_PTR Selected;
  unsigned int Flags;
  unsigned char DefaultButton;
  char *Data;
};

enum { FMT_TRD, FMT_FDI, FMT_SCL, FMT_FDD, FMT_UDI, FMT_TD };

WORD  calculateCheckSum(HoHdr& hdr);
DWORD calculateCheckSum(BYTE* ptr, WORD size);

char* getMsg(int msgId);
void  initDialogItems(InitDialogItem* init, FarDialogItem* item, int noItems);

#endif
