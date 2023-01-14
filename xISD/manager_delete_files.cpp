#include <windows.h>

#include "manager.hpp"
#include "tools.hpp"
#include "iSDOS.hpp"
#include "iSDOS_tools.hpp"
#include "iterator.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

int Manager::deleteOneFile(UniHdr& h, UniHdr* pDir, const char* dirName, int opMode, Action& actProtected, Action& actFolder)
{
  char name[8+3+2];
  makeName(h, name);

  char fullName[300];
  makeFullName(fullName, dirName, name);

  char shortName[40];
  cropLongName(shortName, fullName, 39);
  
  int result = 1;
  if(isDir(h))
  {
    if(actFolder == ASK_USER)
    {
      char *msgItems[7];
      msgItems[0] = getMsg(MDeleteFolder);
      msgItems[1] = getMsg(MDelFolder);
      msgItems[2] = shortName;
      msgItems[3] = getMsg(MDeleteButton);
      msgItems[4] = getMsg(MAll);
      msgItems[5] = getMsg(MSkip);
      msgItems[6] = getMsg(MCancel);
      int askCode = messageBox(FMSG_WARNING | FMSG_DOWN,
                               msgItems,
                               sizeof(msgItems)/sizeof(msgItems[0]), 4);
      switch(askCode)
      {
        case -1:
        case 3:
                  return -1;
        case 2:
                  return 0;
        case 1:
                  actFolder = DO_ALL;
      }
    }
    UniHdr curDir[128];
    readFolder(h, curDir);
    
    for(int i = 1; i < curDir[0].dir.totalFiles; ++i)
      if(curDir[i].attr & FLAG_EXIST)
      {
        
        int tmp_result = deleteOneFile(curDir[i], curDir, fullName, opMode, actProtected, actFolder);
        if(tmp_result == 1) continue;
        
        result = tmp_result;
        if(result == -1) break;
      }
    if(result == 1)
    {
      h.attr &= ~FLAG_EXIST;
      curDir[0].attr &= ~FLAG_EXIST;
      Iterator cur(filer, img, h), end;
      while(cur != end)
      {
        unmarkBlock(*cur);
        ++cur;
      }
      if(!(h.attr & FLAG_SOLID)) unmarkBlock(h.dir.descr1stBlock);
      
      --pDir[0].dir.noFiles;
    }
    writeFolder(curDir);
  }
  else
  {
    if(h.attr == 0xFF && h.file.systemFlag == 0xFF) return 0;
    if(h.attr & FLAG_DELETE_PROTECT)
    {
      if(actProtected == SKIP_ALL)  return 0;
      if((opMode & OPM_SILENT) == 0 && actProtected == ASK_USER)
      {
        char *msgItems[9];
        msgItems[0] = getMsg(MWarning);
        msgItems[1] = getMsg(MFileProtected);
        msgItems[2] = shortName;
        msgItems[3] = getMsg(MDelProtected);
        msgItems[4] = getMsg(MDeleteButton);
        msgItems[5] = getMsg(MAll);
        msgItems[6] = getMsg(MSkip);
        msgItems[7] = getMsg(MSkipAll);
        msgItems[8] = getMsg(MCancel);
        int askCode = messageBox(FMSG_WARNING | FMSG_DOWN,
                                 msgItems,
                                 sizeof(msgItems)/sizeof(msgItems[0]), 5);
        switch(askCode)
        {
          case -1:
          case 4:
                    return -1;
          case 3:
                    actProtected = SKIP_ALL;
          case 2:
                    return 0;
          case 1:
                    actProtected = DO_ALL;
        }
      }
    }
    if((opMode & OPM_SILENT) == 0)
    {
      char *msgItems[3];
      msgItems[0] = getMsg(MDelete);
      msgItems[1] = getMsg(MDeleting);
      msgItems[2] = shortName;
      messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);
    }
    --pDir[0].dir.noFiles;
    h.attr &= ~FLAG_EXIST;

    Iterator cur(filer, img, h), end;
    while(cur != end)
    {
      unmarkBlock(*cur);
      ++cur;
    }
    if(!(h.attr & FLAG_SOLID)) unmarkBlock(h.file.firstBlock);
  }
  return result;
}

int Manager::deleteFiles(PluginPanelItem *panelItem, int noItems, int opMode)
{
  if(noItems == 0) return TRUE;
  readInfo();

  if((opMode & OPM_SILENT) == 0)
  {
    char *msgItems[5];
    msgItems[0] = getMsg(MDelete);
    msgItems[1] = getMsg(MDelFile);
    msgItems[3] = getMsg(MDeleteButton);
    msgItems[4] = getMsg(MCancel);

    char msg[30];
    if(noItems == 1)
      wsprintf(msg, "%s", panelItem[0].FindData.cFileName);
    else
      wsprintf(msg, getMsg(MDelFiles), noItems);

    msgItems[2] = msg;
    if(messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 2)!=0) return FALSE;
    if(noItems > 1)
    {
      char *msgItems[5];
      msgItems[0] = getMsg(MDeleteFiles);
      msgItems[1] = getMsg(MDelFile);
      msgItems[3] = getMsg(MAll);
      msgItems[4] = getMsg(MCancel);

      char msg[30];
      wsprintf(msg, getMsg(MDelFiles), noItems);
      msgItems[2] = msg;
      if(messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 2)!=0)
        return FALSE;
    }
  }
  if(!openHostFile()) return FALSE;

  HANDLE screen = startupInfo.SaveScreen(0, 0, -1, -1);

  Action  actProtected = ASK_USER;
  Action  actFolder    = ASK_USER;
  int     exitCode     = TRUE;
  
  for(int iNum = 0; iNum < noItems; ++iNum)
  {
    int fNum = findFile(files, panelItem[iNum].FindData.cFileName);
    if(!fNum) continue;

    int result = deleteOneFile(files[fNum], files, curDir, opMode, actProtected, actFolder);

    if(result == -1)
    {
      exitCode = FALSE;
      break;
    }
  }

  startupInfo.RestoreScreen(screen);  

  writeFolder(files);
  write_device_sys();
  closeHostFile();
  op.reread = true;
  startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
  op.reread = false;
  return exitCode;
}
