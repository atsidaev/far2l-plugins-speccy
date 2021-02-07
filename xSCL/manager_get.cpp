#include <windows.h>
#include "plugin.hpp"

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern Options op;
extern PluginStartupInfo startupInfo;

int Manager::createFile(HANDLE& file, char* name, int& action)
{
  // проверяем наличие файла
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(name, &data);
  if(h != INVALID_HANDLE_VALUE)
  {
    // файл с таким именем существует
    FindClose(h);
    if(action == 2) return 0;
    if(action == 0)
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

      char param[50];
      if(data.ftLastWriteTime.dwLowDateTime == 0 &&
         data.ftLastWriteTime.dwHighDateTime == 0)
      {
        wsprintf(param, getMsg(MDestination1), data.nFileSizeLow);
      }
      else
      {
        FILETIME lastWriteTime;
        FileTimeToLocalFileTime(&data.ftLastWriteTime, &lastWriteTime);
        
        SYSTEMTIME time;
        FileTimeToSystemTime(&lastWriteTime, &time);
        
        wsprintf(param, getMsg(MDestination2),
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
                  return -1;
        case 3:
                  action = 2; //skipAll
        case 2:
                  return 0;
        case 1:
                  action = 1; //overwriteAll
      }
    }
  }
  file = CreateFile(name,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL);
  if(file != INVALID_HANDLE_VALUE) return 1;
  
  char *msgItems[4];
  msgItems[0] = getMsg(MError);
  msgItems[1] = getMsg(MCanNotCreate);
  msgItems[2] = name;
  msgItems[3] = getMsg(MOk);
  messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
  return 0;
}

int Manager::getFiles(PluginPanelItem *panelItem,
                      int noItems,
                      int move,
                      char *destPath,
                      int opMode)
{
  if(noItems == 0) return 1;
  if(isReadOnly && move)
  {
    char *msgItems[4];
    msgItems [0] = getMsg(MWarning);
    msgItems [1] = getMsg(MCanNotMove);
    msgItems [2] = getMsg(MWriteProtected);
    msgItems [3] = getMsg(MOk);
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
    wsprintf(msg,
             getMsg(MFileTo),
             move ? action2[1] : action2[0],
             panelItem[0].FindData.cFileName);
  else
    wsprintf(msg, getMsg(MFilesTo), move ? action2[1] : action2[0], noItems);

  char historyName[] = "xSCL_copy_path";
  
  InitDialogItem items[]={
    DI_DOUBLEBOX,3,1,72,11,0,0,0,0, move ? action[1] : action[0],
    DI_TEXT,5,2,0,0,0,0,0,0,msg,
    DI_EDIT,5,3,70,3,1,(int)historyName,DIF_HISTORY,0,destPath,
    DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_RADIOBUTTON,5,5,0,0,0,0,DIF_GROUP,0,(char*)MHoBeta,
    DI_RADIOBUTTON,5,6,0,0,0,0,0,0,(char*)MSCL,
    DI_TEXT,3,7,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_CHECKBOX,5,8,0,0,0,0,0,0,(char*)MSkipHeaders,
    DI_TEXT,3,9,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,0,10,0,0,0,0,DIF_CENTERGROUP,1, move ? button[1] : button[0],
    DI_BUTTON,0,10,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
  };
  
  FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
  initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
  
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
  
  bool singleFile = false, skipHeader = false;
  // если надо показать диалог, то покажем
  if((opMode & OPM_SILENT) == 0)
  {
    int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                     -1, -1, 76, 13,
                                     NULL,
                                     dialogItems,
                                     sizeof(dialogItems)/sizeof(dialogItems[0]));
    if(askCode != 9) return -1;
    lstrcpy(destPath, dialogItems[2].Data);
    
    singleFile  = (dialogItems[4].Selected == FALSE);
    skipHeader = dialogItems[7].Selected;
  }
  
  // если пользователь хочет, то создадим каталоги
  if(GetFileAttributes(destPath)==0xFFFFFFFF)
    for(char *c=destPath; *c; c++)
    {
      if(*c!=' ')
      {
        for(; *c; c++)
          if(*c=='\\')
          {
            *c=0;
            CreateDirectory(destPath, NULL);
            *c='\\';
          }
        CreateDirectory(destPath, NULL);
        break;
      }
    }
  if(*destPath && destPath[lstrlen(destPath)-1] != ':') addEndSlash(destPath);
  
  DWORD noBytesWritten;
  DWORD checkSum     = 0;
  
  int buttonStatus = 0; // состояние кнопок overwrite_all/skip_all
  
  HANDLE file;
  char   name[300];
  
  if(singleFile)
  {
    wsprintf(name, "%s%s", destPath, panelItem[0].FindData.cFileName);
    *pointToExt(name) = 0;
    if(skipHeader)
      lstrcat(name, "bin");
    else
      lstrcat(name, "scl");
    int r = createFile(file, name, buttonStatus);
    if(r == 0 || r == -1) return -1;
    if(!skipHeader) checkSum = writeSCLHeader(file, noItems);
  }
  
  int  exitCode       = 1;
  BYTE noFilesWritten = 0;
  
  if(!openHostFile()) return 0;
  
  HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);

  for(int iNum = 0; iNum < noItems; ++iNum)
  {
    // определяем номер файла
    int fNum;
    for(fNum = 0; fNum < noFiles; ++fNum)
      if(!lstrcmp(panelItem[iNum].FindData.cFileName, pcFiles[fNum].name)) break;

    // просмотр текстовых файлов без заголовка
    if(noItems == 1 && ((opMode & OPM_VIEW) || (opMode & OPM_EDIT)) && pcFiles[fNum].skipHeader) skipHeader = true;

    if(!singleFile)
    {
      wsprintf(name, "%s%s", destPath, panelItem[iNum].FindData.cFileName);
      if(skipHeader) lstrcat(name, ".bin");
      int r = createFile(file, name, buttonStatus);
      if(r == -1)
      {
        exitCode = -1;
        break;
      }
      if(r == 0)
      {
        exitCode = 0;
        continue;
      }
      if(!skipHeader)
      {
        HoHdr hdr;
        CopyMemory(hdr.name, files[fNum].name, 8);
        
        hdr.type     = files[fNum].type;
        hdr.start    = files[fNum].start;
        hdr.size     = files[fNum].size;
        hdr.reserved = 0;
        hdr.noSecs   = files[fNum].noSecs;
        hdr.checkSum = calculateCheckSum(hdr);

        WriteFile(file, &hdr, sizeof(HoHdr), &noBytesWritten, NULL);
      }
    }
    else
    {
      if(!skipHeader)
      {
        // пишем заголовок
        SetFilePointer(file, iNum*sizeof(FileHdr)+8+1, NULL, FILE_BEGIN);
        WriteFile     (file, &files[fNum], sizeof(FileHdr), &noBytesWritten, NULL);
        checkSum += calculateCheckSum((BYTE*)&files[fNum], sizeof(FileHdr));
      }
    }
    
    char* msgItems[2];
    msgItems[0] = getMsg(MCopyingFile);
    char  shortName[51] = "...";
    msgItems[1] = name;
    if(lstrlen(name) > 47)
    {
      lstrcat(shortName, name + lstrlen(name) - 47);
      msgItems[1] = shortName;
    }
    if((opMode & OPM_SILENT) == 0)
      messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);

    // копируем тело
    checkSum += copyFile(fNum, file, skipHeader);
    if(!singleFile) CloseHandle(file);
    
    // пометили файл как успешно скопированный
    panelItem[iNum].Flags ^= PPIF_SELECTED;
    if(move) pcFiles[fNum].deleted = true;
    ++noFilesWritten;
  }
  closeHostFile();

  startupInfo.RestoreScreen(screen);

  if(singleFile)
  {
    if(!skipHeader)
      WriteFile(file, &checkSum, sizeof(checkSum), &noBytesWritten, NULL);
    CloseHandle(file);
  }
  if(move) deleteFilesImpl(noFilesWritten);
  
  return exitCode;
}
