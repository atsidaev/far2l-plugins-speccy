#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "manager.hpp"
#include "tools.hpp"
#include "iSDOS.hpp"
#include "iSDOS_tools.hpp"
#include "iterator.hpp"
#include "lang.hpp"

#include "../shared/widestring.hpp"

extern PluginStartupInfo startupInfo;

int Manager::writeFile(const char *name, const UniHdr& hdr)
{
  u32 attr = FILE_FLAG_SEQUENTIAL_SCAN;
  if(hdr.attr & FLAG_READ_PROTECT  ||
     hdr.attr & FLAG_WRITE_PROTECT ||
     hdr.attr & FLAG_DELETE_PROTECT) attr |= FILE_ATTRIBUTE_READONLY;
  if(hdr.attr & FLAG_HIDDEN)         attr |= FILE_ATTRIBUTE_HIDDEN;

  HANDLE file = CreateFile(_W((char*)name).c_str(),
                           GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           CREATE_ALWAYS,
                           attr,
                           NULL);
  if(file == INVALID_HANDLE_VALUE)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotCreate);
    msgItems[3] = getMsg(MOk);

    char shortName[51];
    msgItems[2] = cropLongName(shortName, name, 50);

    messageBox(FMSG_WARNING | FMSG_DOWN,
               msgItems,
               sizeof(msgItems)/sizeof(msgItems[0]),
               1);
    return 0;
  }

  u32 size = getSize(hdr);
  if(size != 0)
  {
    u8  buf[blockSize];
    DWORD noBytesWritten;
    
    Iterator cur(filer, img, hdr), end;
    while(cur != end)
    {
      readBlock(*cur, buf);
      ++cur;
      if(cur != end)
        WriteFile(file, buf, blockSize, &noBytesWritten, NULL);
      else
      {
        int s = (size%blockSize) ? size%blockSize : blockSize;
        WriteFile(file, buf, s, &noBytesWritten, NULL);
      }
    }
  }
  FILETIME modificationTime;
  memset(&modificationTime, 0, sizeof(modificationTime));
  DosDateTimeToFileTime(hdr.date, hdr.file.time, &modificationTime);
  LocalFileTimeToFileTime(&modificationTime, &modificationTime);

  u16        tmp = hdr.file.loadAddr;
  SYSTEMTIME laddr;
  laddr.wMilliseconds = 0;
  laddr.wSecond       = 0;
  laddr.wMinute       = tmp%60;
  tmp /= 60;
  laddr.wHour         = tmp%24;
  tmp /= 24;
  laddr.wDay          = tmp%31 + 1;
  tmp /= 31;
  laddr.wMonth        = tmp+1;
  laddr.wYear         = 2000;

  FILETIME   creationTime;
  SystemTimeToFileTime(&laddr, &creationTime);
  SetFileTime(file, &creationTime, NULL, &modificationTime);
  CloseHandle(file);
  return 1;
}

int isFileExist(const char* name, const UniHdr& hdr, Action& action)
{
  // проверяем наличие файла
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(_W((char*)name).c_str(), &data);
  if(h != INVALID_HANDLE_VALUE)
  {
    FindClose(h);
    // файл с таким именем существует
    if(action == SKIP_ALL) return 0;
    if(action == ASK_USER)
    {
      char *msgItems[12];
      msgItems[0] = getMsg(MWarning);
      msgItems[1] = getMsg(MAlreadyExists);
      msgItems[3] = "\x001";
      msgItems[6] = "\x001";
      msgItems[7] = getMsg(MOverwrite);
      msgItems[8] = getMsg(MAll);
      msgItems[9] = getMsg(MSkip);
      msgItems[10] = getMsg(MSkipAll);
      msgItems[11] = getMsg(MCancel);
      
      char shortName[50];
      msgItems[2] = cropLongName(shortName, name, 47);

      char dest  [50];
      char source[50];

      SYSTEMTIME st = makeDate(hdr);
      sprintf(source, getMsg(MSrc),
               getSize(hdr),
               st.wDay, st.wMonth, st.wYear,
               st.wHour, st.wMinute, st.wSecond);

      FILETIME lastWriteTime;
      FileTimeToLocalFileTime(&data.ftLastWriteTime, &lastWriteTime);

      SYSTEMTIME time;
      FileTimeToSystemTime(&lastWriteTime, &time);
                     
      sprintf(dest, getMsg(MDest),
               data.nFileSize,
               time.wDay, time.wMonth, time.wYear,
               time.wHour, time.wMinute, time.wSecond);
      
      msgItems[4] = source;
      msgItems[5] = dest;
      int askCode = messageBox(FMSG_WARNING | FMSG_DOWN,
                               msgItems,
                               sizeof(msgItems)/sizeof(msgItems[0]), 5);
      switch(askCode)
      {
        case -1:
        case 4:
                  return -1;
        case 3:
                  action = SKIP_ALL;
        case 2:
                  return 0;
        case 1:
                  action = DO_ALL;
      }
    }
  }
  return 1;
}

int Manager::getOneFile(UniHdr&       h,
                        UniHdr*       pDir,
                        const char*   to_path,
                        const char*   from_path,
                        int           move,
                        Action&       action,
                        int           opMode)
{
  char name[8+3+2];
  makeName(h, (const u8*)name);

  char fullToName[300];
  makeFullName(fullToName, to_path, name);
  
  char fullFromName[300];
  makeFullName(fullFromName, from_path, name);
  
  int result = 1;
  if(isDir(h))
  {
    addEndSlash(fullToName);
    CreateDirectory(_W(fullToName).c_str(), NULL);

    addEndSlash(fullFromName);

    UniHdr curDir[128];
    readFolder(h, curDir);

    for(int i = 1; i < curDir[0].dir.totalFiles; ++i)
      if(curDir[i].attr & FLAG_EXIST)
      {
        int tmp_result = getOneFile(curDir[i], curDir, fullToName, fullFromName, move, action, opMode);
        if(tmp_result == 1) continue;

        result = tmp_result;
        if(result == -1) break;
      }
    if(move) writeFolder(curDir);
  }
  else
  {
    if((opMode & OPM_SILENT) == 0)
    {
      char *msgItems[5];
      msgItems[3] = getMsg(MTo);
      if(move)
      {
        msgItems[0] = getMsg(MMove);
        msgItems[1] = getMsg(MMoving);
      }
      else
      {
        msgItems[0] = getMsg(MCopy);
        msgItems[1] = getMsg(MCopying);
      }

      char shortfullFromName[45];
      char shortfullToName[45];
      msgItems[2] = cropLongName(shortfullFromName, fullFromName, 40);
      msgItems[4] = cropLongName(shortfullToName, fullToName, 40);
      messageBox(FMSG_LEFTALIGN, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);
    }

    result = isFileExist(fullToName, h, action);
    if(result == 1) result = writeFile(fullToName, h);
  }

  Action actProtected = DO_ALL;
  Action actFolder    = DO_ALL;

  if(result == 1 && move)
    deleteOneFile(h, pDir, from_path, opMode | OPM_SILENT, actProtected, actFolder);
  return result;
}

int Manager::getFiles(PluginPanelItem *panelItem,
                      int   noItems,
                      int   move,
                      char* destPath,
                      int   opMode)
{
  if(noItems == 0) return 1;
  addEndSlash(destPath);
  
  char  msg[100] = "";
  char *act[2];
  char *actTo[2];
  char *button[2];
  act[0] = getMsg(MCopy);
  act[1] = getMsg(MMove);
  actTo[0] = getMsg(MCopyTo);
  actTo[1] = getMsg(MMoveTo);
  button[0] = getMsg(MCopyButton);
  button[1] = getMsg(MMoveButton);
  
  if(noItems == 1)
    sprintf(msg, getMsg(MFileTo), move ? actTo[1] : actTo[0], panelItem[0].FindData.cFileName);
  else
    sprintf(msg, getMsg(MFilesTo), move ? actTo[1] : actTo[0], noItems);
  
  char historyName[] = "XiSD_copy_path";
  
  InitDialogItem items[] =
  {
    DI_DOUBLEBOX,3,1,72,6,0,0,0,0, move ? act[1] : act[0],
    DI_TEXT,     5,2,0,0,0,0,0,0,msg,
    DI_EDIT,     5,3,70,3,1,(DWORD_PTR)historyName,DIF_HISTORY,0,destPath,
    DI_TEXT,     3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,   0,5,0,0,0,0,DIF_CENTERGROUP,1, move ? button[1] : button[0],
    DI_BUTTON,   0,5,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
  };
  
  FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
  initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
  
  // если надо показать диалог, то покажем
  if((opMode & OPM_SILENT) == 0)
  {
    int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                     -1, -1, 76, 8,
                                     NULL,
                                     dialogItems,
                                     sizeof(dialogItems)/sizeof(dialogItems[0]));
    if(askCode != 4) return -1;
    strcpy(destPath, dialogItems[2].Data);
  }
  
  // если пользователь хочет, то создадим каталоги
  if(GetFileAttributes(_W(destPath).c_str())==0xFFFFFFFF)
    for(char *c=destPath; *c; c++)
    {
      if(*c!=' ')
      {
        for(; *c; c++)
          if(*c=='\\')
          {
            *c=0;
            CreateDirectory(_W(destPath).c_str(), NULL);
            *c='\\';
          }
        CreateDirectory(_W(destPath).c_str(), NULL);
        break;
      }
    }
  if(*destPath && destPath[strlen(destPath)-1] != ':') addEndSlash(destPath);
  
  if(!openHostFile()) return 0;
  HANDLE screen = startupInfo.SaveScreen(0, 0, -1, -1);

  int    exitCode = 1;
  Action action   = ASK_USER;

  for(int iNum = 0; iNum < noItems; ++iNum)
  {
    int fNum = findFile(files, panelItem[iNum].FindData.cFileName);
    if(!fNum) continue;
    
    int result = getOneFile(files[fNum], files, destPath, curDir, move, action, opMode);
    
    if(result == -1)
    {
      exitCode = -1;
      break;
    }
    if(result ==  0)
    {
      exitCode = 0;
      continue;
    }
    panelItem[iNum].Flags ^= PPIF_SELECTED;
  }

  startupInfo.RestoreScreen(screen);

  if(move)
  {
    writeFolder(files);
    write_device_sys();
  }

  closeHostFile();
  return exitCode;
}
