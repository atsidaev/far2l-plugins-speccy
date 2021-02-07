#include <windows.h>
#include "pluginold.hpp"

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

#include "widestring.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

namespace
{
  enum Action   { ASK_USER, OVERWRITE_ALL, SKIP_ALL };
  Action userAction;

  bool   skipPathnames;
  bool   skipHeaders;
  bool   scl;
  bool   keepSilence;
  
  HANDLE file;
  
  // все что нужно для записи SCL
  BYTE   noFilesWritten;
  
  BYTE*  hdrs;
  BYTE*  bodies;
  DWORD  bodiesSize;
}

ExitCode createFile(char* name)
{
  // проверяем наличие файла
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(_W(name).c_str(), &data);
  if(h != INVALID_HANDLE_VALUE)
  {
    // файл с таким именем существует
    FindClose(h);
    if(userAction == SKIP_ALL) return SKIP;
    if(userAction == ASK_USER)
    {
      char *msgItems[11];
      msgItems[0] = getMsg(MWarning);
      msgItems[1] = getMsg(MAlreadyExists);
      msgItems[2] = name;
      msgItems[3] = "\x001";
      msgItems[5] = "\x001";
      msgItems[6] = getMsg(MOverwrite);
      msgItems[7] = getMsg(MAll);
      msgItems[8] = getMsg(MSkip);
      msgItems[9] = getMsg(MSkipAll);
      msgItems[10] = getMsg(MCancel);
      char shortName[51];
      msgItems[2] = truncPathStr(shortName, name, 50);
      
      char param[50];
      if(data.ftLastWriteTime.dwLowDateTime == 0 &&
         data.ftLastWriteTime.dwHighDateTime == 0)
      {
        sprintf(param, getMsg(MDestination1), data.nFileSizeLow);
      }
      else
      {
        FILETIME lastWriteTime;
        FileTimeToLocalFileTime(&data.ftLastWriteTime, &lastWriteTime);
        
        SYSTEMTIME time;
        FileTimeToSystemTime(&lastWriteTime, &time);
        
        sprintf(param, getMsg(MDestination2),
                 data.nFileSizeLow,
                 time.wDay, time.wMonth, time.wYear,
                 time.wHour, time.wMinute, time.wSecond);
      }
      msgItems[4] = param;
      int askCode = messageBox(FMSG_WARNING | FMSG_DOWN,
                               msgItems,
                               sizeof(msgItems)/sizeof(msgItems[0]), 5);
      switch(askCode)
      {
        case -1:
        case 4:
                  return CANCEL;
        case 3:
                  userAction = SKIP_ALL;
        case 2:
                  return SKIP;
        case 1:
                  userAction = OVERWRITE_ALL;
      }
    }
  }
  file = CreateFile(_W(name).c_str(),
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL);
  if(file != INVALID_HANDLE_VALUE) return OK;
  
  char *msgItems[4];
  msgItems[0] = getMsg(MError);
  msgItems[1] = getMsg(MCanNotCreate);
  msgItems[2] = name;
  msgItems[3] = getMsg(MOk);
  messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
  return SKIP;
}

ExitCode Manager::getFile(int fNum, bool isMove, char* to)
{
  ExitCode exitCode = OK;

  char name[300];
  sprintf(name, "%s%s", to, pcFiles[fNum].name);
  
  char* msgItems[3];
  msgItems[0] = getMsg(MCopyingFile);
  msgItems[1] = getMsg(MCopyTheFile);
  
  char shortName[50];
  msgItems[2] = truncPathStr(shortName, name, 30);
  if(!keepSilence)
    messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);
  
  int lastPartSize = sectorSize;

  if(skipHeaders)
  {
    int size    = files[fNum].size;
    int noSecs2 = size/sectorSize + !!(size%sectorSize);
    if(files[fNum].noSecs == noSecs2 && size%sectorSize) lastPartSize = size % sectorSize;
  }
  
  if(scl)
  {
    hdrs= (BYTE*)realloc(hdrs,   (noFilesWritten+1)*sizeof(SCLHdr));
    memcpy(hdrs+noFilesWritten*sizeof(SCLHdr), &files[fNum], sizeof(SCLHdr));
    if(files[fNum].noSecs)
    {
      bodies = (BYTE*)realloc(bodies, bodiesSize+sectorSize*(files[fNum].noSecs-1)+lastPartSize);

      BYTE  sector[sectorSize];
      BYTE* ptr = bodies + bodiesSize;
      int trk = files[fNum].trk, sec = files[fNum].sec;
      for(int s = 0; s < files[fNum].noSecs-1; ++s)
      {
        read(trk, sec, sector);
        if(++sec == 16) { trk++; sec = 0; }
        memcpy(ptr, sector, sectorSize);
        ptr        += sectorSize;
        bodiesSize += sectorSize;
      }
      // обрабатываем последний сектор хитрым образом
      read(trk, sec, sector);
      memcpy(ptr, sector, lastPartSize);
      bodiesSize += lastPartSize;
    }
    ++noFilesWritten;
  }
  else
  {
    if(skipHeaders) strcat(name, ".bin");
    ExitCode code = createFile(name);
    if(code != OK) return code;

    DWORD noBytesWritten;
    if(!skipHeaders)
    {
      HoHdr hdr;
      memcpy(hdr.name, files[fNum].name, 8);

      hdr.type     = files[fNum].type;
      hdr.start    = files[fNum].start;
      hdr.size     = files[fNum].size;
      hdr.reserved = 0;
      hdr.noSecs   = files[fNum].noSecs;
      hdr.checkSum = calculateCheckSum(hdr);
      WriteFile(file, &hdr, sizeof(HoHdr), &noBytesWritten, NULL);
    }

    if(files[fNum].noSecs)
    {
      // копируем тело файла
      BYTE sector[sectorSize];

      int trk = files[fNum].trk, sec = files[fNum].sec;
      for(int s = 0; s < files[fNum].noSecs-1; ++s)
      {
        read(trk, sec, sector);
        if(++sec == 16) { trk++; sec = 0; }
        WriteFile(file, sector, sectorSize, &noBytesWritten, NULL);
      }
      // обрабатываем последний сектор хитрым образом
      read(trk, sec, sector);
      WriteFile(file, sector, lastPartSize, &noBytesWritten, NULL);
    }
    CloseHandle(file);
  }
  
  // "удаляем" (если надо) успешно скопированный файл
  if(isMove && exitCode == OK)
  {
    if(files[fNum].name[0] != 0x01) ++noDelFiles;
    files[fNum].name[0] = 0x01;
  }
  return exitCode;
}

ExitCode Manager::getFolder(int fNum, bool isMove, char* to)
{
  ExitCode exitCode = OK;

  char dest[300];
  strcpy(dest, to);
  if(!skipPathnames)
  {
    strcat(dest, pcFolders[fNum-1]);
    addEndSlash(dest);
    CreateDirectory(_W(dest).c_str(), NULL);
  }
  // копируем вложенные каталоги
  for(int i = 0; i < noFolders; ++i)
    if(folderMap[i] == fNum)
    {
      ExitCode code = getFolder(i+1, isMove, dest);
      
      if(code == CANCEL) return CANCEL;
      if(code == SKIP)   exitCode = SKIP;
    }

  // копируем вложенные файлы
  for(int i = 0; i < noFiles; ++i)
    if(fileMap[i] == fNum)
    {
      ExitCode code = getFile(i, isMove, dest);
      if(code == CANCEL) return CANCEL;
      if(code == SKIP)   exitCode = SKIP;
    }

  // "удаляем" (если надо) успешно скопированный католог
  if(isMove && exitCode == OK)
  {
    if(folders[fNum-1][0] != 0x01) ++noDelFolders;
    folders[fNum-1][0] = 0x01;
  }
  return exitCode;
}

int Manager::getFiles(PluginPanelItem *panelItem,
                      int noItems,
                      int isMove,
                      char *destPath,
                      int opMode)
{
  if(noItems == 0) return 1;
  if(fmt->isProtected(img) && isMove)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MWarning);
    msgItems[1] = getMsg(MCanNotMove);
    msgItems[2] = getMsg(MWriteProtected);
    msgItems[3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return FALSE;
  }

  if(*destPath) addEndSlash(destPath);
  
  char msg[100];
  char *action[2];
  char *action2[2];
  char *button[2];
  action[0] = getMsg(MCopy);
  action[1] = getMsg(MMove);
  action2[0] = getMsg(MCopying);
  action2[1] = getMsg(MMoving);
  button[0] = getMsg(MCopyButton);
  button[1] = getMsg(MMoveButton);
  if(noItems == 1)
    sprintf(msg, getMsg(MFileTo), action2[!!isMove], panelItem[0].FindData.cFileName);
  else
    sprintf(msg, getMsg(MFilesTo), action2[!!isMove], noItems);

  const char* historyName = "xTRD_copy_path";
  
  InitDialogItem items[]={
    DI_DOUBLEBOX,3,1,72,12,0,0,0,0, action[!!isMove],
    DI_TEXT,5,2,0,0,0,0,0,0,msg,
    DI_EDIT,5,3,70,0,1,(DWORD_PTR)historyName,DIF_HISTORY,0,destPath,
    DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_RADIOBUTTON,5,5,0,0,0,0,DIF_GROUP,0,(char*)MHoBeta,
    DI_RADIOBUTTON,5,6,0,0,0,0,0,0,(char*)MSCL,
    DI_TEXT,3,7,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_CHECKBOX,5,8,0,0,0,0,0,0,(char*)MSkipHeaders,
    DI_CHECKBOX,5,9,0,0,0,0,0,0,(char*)MCopyWithoutPathnames,
    DI_TEXT,3,10,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,0,11,0,0,0,0,DIF_CENTERGROUP,1, button[!!isMove],
    DI_BUTTON,0,11,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
  };
  
  FarDialogItem dialogItems[ARRAYSIZE(items)];
  memset(dialogItems, 0, sizeof(FarDialogItem) * ARRAYSIZE(items));
  initDialogItems(items, dialogItems, ARRAYSIZE(items));
  
  switch(op.defaultFormat)
  {
    case 1:
              dialogItems[4].Selected = TRUE;
              break;
    case 2:
              dialogItems[5].Selected = TRUE;
              break;
    default:
              if(noItems == 1)
                dialogItems[4].Selected = TRUE;
              else
                dialogItems[5].Selected = TRUE;
  }
  
  scl           = false;
  skipHeaders   = false;
  skipPathnames = false;
  keepSilence   = opMode & OPM_SILENT;

  // если надо показать диалог, то покажем
  if(!keepSilence)
  {
    int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                     -1, -1, 76, 14,
                                     NULL,
                                     dialogItems,
                                     ARRAYSIZE(dialogItems));
    if(askCode != 10) return -1;
    strcpy(destPath, dialogItems[2].Data);
    
    scl           = !dialogItems[4].Selected;
    skipHeaders   =  dialogItems[7].Selected;
    skipPathnames =  dialogItems[8].Selected;
  }
  
  // если пользователь хочет, то создадим каталоги
  if(GetFileAttributes(_W(destPath).c_str())==0xFFFFFFFF)
    for(char *c=destPath; *c; c++)
    {
      if(*c!=' ')
      {
        for(; *c; c++)
          if(*c=='/')
          {
            *c=0;
            CreateDirectory(_W(destPath).c_str(), NULL);
            *c='/';
          }
        CreateDirectory(_W(destPath).c_str(), NULL);
        break;
      }
    }
  if(*destPath && destPath[strlen(destPath)-1] != ':') addEndSlash(destPath);

  if(!openHostFile()) return 0;

  userAction = ASK_USER;
  
  if(scl)
  {
    skipPathnames =  true;
    noFilesWritten = 0;
    
    char name[300];
    sprintf(name, "%s%s", destPath, panelItem[0].FindData.cFileName);
    
    char* ptr = pointToName(name);
    while(*ptr && *ptr != '.') ++ptr;
    *ptr = 0;
    
    if(skipHeaders)
      strcat(name, ".bin");
    else
      strcat(name, ".scl");
    
    ExitCode code = createFile(name);
    
    if(code != OK) return -1;
    
    hdrs           = (BYTE*)malloc(1);
    bodies         = (BYTE*)malloc(1);
    bodiesSize     = 0;
  }
  
  int returnCode = 1;
  
  HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
  
  for(int iNum = 0; iNum < noItems; ++iNum)
  {
    ExitCode exitCode;
    int fNum;
    if(panelItem[iNum].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      for(fNum = 0; fNum < noFolders; ++fNum)
      {
        if(folderMap[fNum] != curFolderNum) continue;
        if(!strcmp(pcFolders[fNum], panelItem[iNum].FindData.cFileName)) break;
      }
      exitCode = getFolder(fNum+1, isMove, destPath);
    }
    else
    {
      for(fNum = 0; fNum < noFiles; ++fNum)
        if(!strcmp(panelItem[iNum].FindData.cFileName, pcFiles[fNum].name)) break;

      // просмотр текстовых файлов без заголовка
      if(noItems == 1 && ((opMode & OPM_VIEW) || (opMode & OPM_EDIT)) && pcFiles[fNum].skipHeader) skipHeaders = true;
      exitCode = getFile(fNum, isMove, destPath);
    }

    if(exitCode == SKIP)
    {
      returnCode = 0;
      continue;
    }
    if(exitCode == CANCEL)
    {
      returnCode = -1;
      break;
    }
    // пометили файл/католог как успешно скопированный
    panelItem[iNum].Flags ^= PPIF_SELECTED;
  }
  startupInfo.RestoreScreen(screen);

  if(scl)
  {
    char* msgItems[2];
    msgItems[0] = getMsg(MWaiting);
    msgItems[1] = getMsg(MWritingSCL);
    if(!keepSilence)
    {
      screen = startupInfo.SaveScreen(0,0,-1,-1);
      messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);
      startupInfo.RestoreScreen(screen);
    }
    
    DWORD noBytesWritten;
    DWORD checkSum = 0x255 + noFilesWritten;
    BYTE  signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R' };
    if(!skipHeaders)
    {
      WriteFile(file, signature, sizeof(signature), &noBytesWritten, NULL);
      WriteFile(file, &noFilesWritten, 1, &noBytesWritten, NULL);
      checkSum += calculateCheckSum(hdrs, noFilesWritten*sizeof(SCLHdr));
      WriteFile(file, hdrs, noFilesWritten*sizeof(SCLHdr), &noBytesWritten, NULL);
      checkSum += calculateCheckSum(bodies, bodiesSize);
    }
    WriteFile(file, bodies, bodiesSize, &noBytesWritten, NULL);

    if(!skipHeaders)
      WriteFile(file, &checkSum, sizeof(checkSum), &noBytesWritten, NULL);
    
    CloseHandle(file);
    free(bodies);
    free(hdrs);
  }
  if(isMove)
  {
    if(op.autoMove)
      move();
    else
      diskInfo.noDelFiles = noDelFiles;
    
    writeInfo();
  }
  closeHostFile();
  return returnCode;
}
