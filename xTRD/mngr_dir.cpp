#include <windows.h>
#include "plugin.hpp"

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

int Manager::setDirectory(char* dirName, int opMode)
{
  if(!op.useDS || !dsOk) return FALSE;

  if(!lstrcmp(dirName, "\\"))
  {
    *curFolder   = 0;
    curFolderNum = 0;
    return TRUE;
  }
  if(!lstrcmp(dirName, ".."))
  {
    if(*curFolder == 0) return FALSE;

    char* slash = str_r_chr(curFolder, '\\');
    if(slash)
    {
      *slash       = 0;
      curFolderNum = folderMap[curFolderNum-1];
    }
    else
    {
      *curFolder   = 0;
      curFolderNum = 0;
    }
  }
  else
  {
    int  fNum = 0;
    for(;fNum < noFolders; ++fNum)
    {
      if(folderMap[fNum] != curFolderNum) continue;
      if(!lstrcmp(pcFolders[fNum], dirName)) break;
    }
    if(fNum == noFolders) return FALSE;

    curFolderNum = fNum + 1;
    lstrcat(curFolder, "\\");
    lstrcat(curFolder, dirName);
  }
  return TRUE;
}

int Manager::makeDirectory(char* dirName, int opMode)
{
  if(!op.useDS) return -1;
  if(fmt->isProtected(img))
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MWarning);
    msgItems[1] = getMsg(MCanNotMakeDir);
    msgItems[2] = getMsg(MWriteProtected);
    msgItems[3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return FALSE;
  }
  if(!dsOk)
  {
    char *msgItems[5];
    msgItems[0] = getMsg(MWarning);
    msgItems[1] = getMsg(MDSNotInstalled);
    msgItems[2] = getMsg(MInstallDS);
    msgItems[3] = getMsg(MOk);
    msgItems[4] = getMsg(MCancel);
    if(messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 2) != 0) return -1;
    dsOk = true;
  }
  if(noFolders == 127)
  {
    errorMessage(getMsg(MTooManyFolders));
    return -1;
  }
  
  char historyName[] = "xTRD_folder_name";
  InitDialogItem items[] =
  {
    DI_DOUBLEBOX,3,1,60,6,0,0,0,0,(char*)MMakeFolder,
    DI_TEXT,5,2,0,0,0,0,0,0,(char*)MCreateFolder,
    DI_EDIT,5,3,58,0,1,(int)historyName,DIF_HISTORY,0,dirName,
    DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,1,(char*)MOk,
    DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
  };
  
  FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
  initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
  
  // если надо показать диалог, то покажем
  if((opMode & OPM_SILENT) == 0)
  {
    int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                     -1, -1, 64, 8,
                                     NULL,
                                     dialogItems,
                                     sizeof(dialogItems)/sizeof(dialogItems[0]));
    lstrcpy(dirName, dialogItems[2].Data);
    if(askCode != 4) return -1;
  }
  if(*dirName == 0) return 1;

  if(!openHostFile()) return 0;
  FillMemory(folders[noFolders], 11, ' ');

  make8x3name(dirName, folders[noFolders]);
  if(folders[noFolders][0] == 0x01) ++noDelFolders;
  folderMap[noFolders] = curFolderNum;
  ++noFolders;

  writeInfo();
  closeHostFile();
  startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
  return 1;
}
