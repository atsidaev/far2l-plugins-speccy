#include <windows.h>
#include "plugin.hpp"
#include "tools.hpp"
#include "trdos.hpp"

extern PluginStartupInfo info;

WORD calculateCheckSum(HoHdr& hdr)
{
 WORD  sum = 0;
 BYTE* ptr = (BYTE*)&hdr;
 
 for(int i = 0; i < sizeof(HoHdr)-2; i++) sum += ptr[i];
 return (257*sum + 105);
}

DWORD calculateCheckSum(BYTE* ptr, WORD size)
{
  DWORD sum = 0;
  while(size--) sum += *ptr++;
  return sum;
}

char* getMsg(int msgId)
{
 return info.GetMsg(info.ModuleNumber, msgId);
}

void initDialogItems(InitDialogItem* init, FarDialogItem* item, int noItems)
{
  for(int i=0; i < noItems; ++i)
  {
    item[i].Type          = init[i].Type;
    item[i].X1            = init[i].X1;
    item[i].Y1            = init[i].Y1;
    item[i].X2            = init[i].X2;
    item[i].Y2            = init[i].Y2;
    item[i].Focus         = init[i].Focus;
    item[i].Selected      = init[i].Selected;
    item[i].Flags         = init[i].Flags;
    item[i].DefaultButton = init[i].DefaultButton;
    if((unsigned int)init[i].Data < 2000)
      lstrcpy(item[i].Data, getMsg((unsigned int)init[i].Data));
    else
      lstrcpy(item[i].Data, init[i].Data);
  }
}
