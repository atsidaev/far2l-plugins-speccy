#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "tools.hpp"
#include "types.hpp"
#include "lang.hpp"


extern PluginStartupInfo startupInfo;
extern Options           op;

void errorMessage(char *msg)
{
  char *msgItems[3];
  msgItems[0] = getMsg(MError);
  msgItems[1] = msg;
  msgItems[2] = getMsg(MOk);
  startupInfo.Message(startupInfo.ModuleNumber,
                      FMSG_WARNING, NULL,
                      msgItems,
                      sizeof(msgItems)/sizeof(msgItems[0]), 1);
}

bool isEnAlphaNum(BYTE ch)
{
  if(ch >= '0' && ch <= '9') return true;
  if(ch >= 'A' && ch <= 'Z') return true;
  if(ch >= 'a' && ch <= 'z') return true;
  return false;
}

void makeTrDosName(char* dest, const FileHdr& hdr, int width)
{
  for(int i = 0; i < 8; ++i) dest[i] = hdr.name[i] ? hdr.name[i] : ' ';
  
  BYTE ch1, ch2, ch3;
  ch1 = hdr.type;
  ch2 = hdr.start & 0xff;
  ch3 = hdr.start>>8;
  
  if(op.showExt &&
     isEnAlphaNum(ch1) &&
     isEnAlphaNum(ch2) &&
     isEnAlphaNum(ch3))
  {
    dest[width-3] = ch1;
    dest[width-2] = ch2;
    dest[width-1] = ch3;
  }
  else
  {
    dest[width-3] = '<';
    dest[width-2] = ch1 ? ch1 : ' ';
    dest[width-1] = '>';
  }
  dest[width] = 0;
}

char* getMsg(int msgId)
{
  return((char*)startupInfo.GetMsg(startupInfo.ModuleNumber, msgId));
}

void initDialogItems(InitDialogItem *init, FarDialogItem *item, int noItems)
{
  for(int i = 0; i < noItems; i++)
  {
    item[i] = {};
    item[i].Type          = init[i].Type;
    item[i].X1            = init[i].X1;
    item[i].Y1            = init[i].Y1;
    item[i].X2            = init[i].X2;
    item[i].Y2            = init[i].Y2;
    item[i].Focus         = init[i].Focus;
    item[i].Reserved      = init[i].Selected;
    item[i].Flags         = init[i].Flags;
    item[i].DefaultButton = init[i].DefaultButton;
    if((unsigned long)init[i].Data < 2000)
      snprintf(item[i].Data, sizeof(item[i].Data), "%s", getMsg((unsigned long)init[i].Data));
    else
      snprintf(item[i].Data, sizeof(item[i].Data), "%s", init[i].Data);
  }
}

WORD fileSize(const FileHdr& hdr)
{
  WORD size;
  if(hdr.type == 'b' || hdr.type == 'B')
    size = 4 + hdr.start;
  else
    size = hdr.size;
  WORD noSecs = size/256 + (size%256 ? 1 : 0);
  if(noSecs != hdr.noSecs) size = 256*hdr.noSecs;
  return size;
}

