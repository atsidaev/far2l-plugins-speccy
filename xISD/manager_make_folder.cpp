#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "manager.hpp"
#include "tools.hpp"
#include "iSDOS.hpp"
#include "iSDOS_tools.hpp"
#include "iterator.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;

int Manager::makeFolder(UniHdr* pDir, const char* name)
{
  bool isChic;
  if(dsk.signatureChic[0] == 'D' && dsk.signatureChic[1] == 'S' && dsk.signatureChic[2] == 'K')
    isChic = true;
  else
    isChic = false;
  if(isChic)
  {
    if(pDir[0].dir.level == 6 ||
       pDir[0].dir.levelChic == 6 ||
       pDir[0].dir.noFiles == 127) return 0;
  }
  else
    if(pDir[0].dir.level == 6 ||
       pDir[0].dir.noFiles == 127) return 0;
  if(!reserveSpaceForFiles(pDir, 1)) return -1;
  if(noFreeBlocks < 2) return -1;

  UniHdr newDir;
  memset(&newDir, 0, sizeof(UniHdr));
  memset(&newDir, ' ', 8+3);

  for(int i = 0; i < 8; ++i)
  {
    if(!name[i]) break;
    if(isValidChar(name[i]))
      newDir.name[i] = name[i];
    else
      newDir.name[i] = '_';
  }
  
  newDir.attr                  = 0x21;
  newDir.dir.parentDir1stBlock = pDir[0].dir.firstBlock;
  newDir.dir.size              = blockSize;
  if(isChic)
    if(pDir[0].dir.level != 0)
      newDir.dir.level         = pDir[0].dir.level + 1;
    if(pDir[0].dir.levelChic != 0)
      newDir.dir.levelChic     = pDir[0].dir.levelChic + 1;
    if(pDir[0].dir.level == 0 && pDir[0].dir.levelChic == 0)
      newDir.dir.levelChic     = pDir[0].dir.levelChic + 1;
  else
    newDir.dir.level           = pDir[0].dir.level + 1;

  newDir.dir.firstBlock        = findFreeBlocks(1);
  markBlock(newDir.dir.firstBlock);

  newDir.dir.descr1stBlock     = findFreeBlocks(1);
  markBlock(newDir.dir.descr1stBlock);

  u8 block[blockSize];
  memset(block, 0, blockSize);
  block[0] = 1;
  *(u16*)(block+1) = newDir.dir.firstBlock;
  block[3] = 1;
  if(!writeBlock(newDir.dir.descr1stBlock, block))
  {
    read_device_sys();
    return -1;
  }
  
  newDir.dir.totalFiles        = 1;
  newDir.dir.noFiles           = 0;

  SYSTEMTIME time;
  GetLocalTime(&time);
  setDate(newDir, time);
  
  u8 buf[blockSize];
  memset(buf, 0, blockSize);
  memcpy(buf, &newDir, sizeof(UniHdr));
  writeFolder((UniHdr*)buf);
  int fNum = 1;
  for(; fNum < pDir[0].dir.totalFiles; ++fNum)
    if(!(pDir[fNum].attr & FLAG_EXIST)) break;
  
  memcpy(&pDir[fNum], &newDir, sizeof(UniHdr));
  if(fNum == pDir[0].dir.totalFiles) ++pDir[0].dir.totalFiles;
  ++pDir[0].dir.noFiles;

  writeFolder(pDir);
  write_device_sys();
  return 1;
}

int Manager::makeDirectory(char *dirName, int opMode)
{
  // TODO: why it craches when string is here?
  char* historyName = 0; //"XiSD_folder_name";
  InitDialogItem items[] =
  {
    DI_DOUBLEBOX,3,1,60,6,0,0,0,0,(char *)MMakeFolder,
    DI_TEXT,5,2,0,0,0,0,0,0,(char *)MCreateFolder,
    DI_EDIT,5,3,58,3,1,(DWORD_PTR)historyName,DIF_HISTORY,0,dirName,
    DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,1,(char*)MOk,
    DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
  };
  
  FarDialogItem dialogItems[ARRAYSIZE(items)];
  memset(dialogItems, 0, sizeof(FarDialogItem) * ARRAYSIZE(items));
  initDialogItems(items, dialogItems, ARRAYSIZE(items));
  
  // если надо показать диалог, то покажем
  if((opMode & OPM_SILENT) == 0)
  {
    int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                     -1, -1, 64, 8,
                                     NULL,
                                     dialogItems,
                                     sizeof(dialogItems)/sizeof(dialogItems[0]));
    strcpy(dirName, dialogItems[2].Data);
    if(askCode != 4) return -1;
  }
  char *p = dirName;
  if(*p == 0) return 1;

  char cname[8+3+2] = "";
  makeCompatibleFolderName(cname, dirName);

  int fNum = findFile(files, cname);
  if(fNum) return 0;
  if(!openHostFile()) return 0;
  int result = makeFolder(files, cname);

  closeHostFile();
  startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
  return result;
}
