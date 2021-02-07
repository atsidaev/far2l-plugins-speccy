#include <windows.h>
#include "pluginold.hpp"
using namespace oldfar;
#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

const DWORD PKF_ANY = PKF_CONTROL | PKF_SHIFT | PKF_ALT;

int str2int(char* p, int len)
{
  int result = -1;
  while(*p == ' ') ++p;
  
  for(int i = 0; i < len; ++i)
  {
    BYTE digit = 0xFF;
    if(*p >= '0' && *p <= '9') digit = *p - '0';

    if(digit == 0xFF) break;
    if(result == -1) result = 0;
    
    result = 10*result + digit;
    p++;
  }
  return result;
}

char* appendFileName(char* fileName, char* to, bool shortName)
{
  char buf[300];
  // TODO: restore or remove (atsidaev)
/*  if(shortName)
    GetShortPathName(fileName, buf, sizeof(buf));
  else*/
  {
    strcpy(buf, fileName);
    quoteSpaceOnly(buf);
  }
  
  memcpy(to, buf, strlen(buf));
  return to+strlen(buf);
}

int Manager::processKey(int key, unsigned int controlState)
{
  if(key == VK_RETURN)
  {
    char buf[1024];
    startupInfo.Control(this,FCTL_GETCMDLINE, buf);
    if(*buf) return FALSE;
    
    HANDLE hScreen = startupInfo.SaveScreen(0,0,-1,-1);
    PanelInfo pInfo;
    startupInfo.Control(this,FCTL_GETPANELINFO,&pInfo);
    startupInfo.RestoreScreen(hScreen);

    if(pInfo.ItemsNumber <= 0 || pInfo.CurrentItem == 0) return FALSE;
    char* name = pInfo.PanelItems[pInfo.CurrentItem].FindData.cFileName;
    int fNum = 0;
    for(; fNum < noFiles; ++fNum)
      if(!strcmp(name, pcFiles[fNum].name)) break;
    if(fNum == noFiles) return FALSE;

    char  cmdLine[600];
    char* p          = op.cmdLine1;
    char* type       = op.types1;
    bool  fullScreen = op.fullScreen1;

    if(controlState & PKF_CONTROL)
    {
      p          = op.cmdLine2;
      type       = op.types2;
      fullScreen = op.fullScreen2;
    }

    char* ptr      = pcFiles[fNum].name;
    char  fileType = ' ';
    
    if(*type == '*') goto OkType;
    
    while(*ptr && *ptr != '.') ++ptr;
    if(*ptr) fileType = *(ptr+2);
      
    while(*type)
    {
      if(*type == fileType) goto OkType;
      ++type;
    }
    return FALSE;
OkType:
    char* to = cmdLine;
    while(*p)
    {
      if(*p != '%')
        *to++ = *p++;
      else
      {
        switch(*(p+1))
        {
          case 'P':
                    to = appendFileName(hostFileName, to, false);
                    break;
          case 'p':
                    to = appendFileName(hostFileName, to, true);
                    break;
          case 'D':
                    to = appendFileName(pointToName(hostFileName), to, false);
                    break;
          case 'd':
                    to = appendFileName(pointToName(hostFileName), to, true);
                    break;
          case 'f':
                    memcpy(to, files[fNum].name, 8);
                    to += 8;
                    *to++ = '.';
                    *to++ = files[fNum].type;
                    break;
          case 'N':
                    to += sprintf(to, "%d", fNum+1);
                    break;
          case 'n':
                    to += sprintf(to, "%d", fNum);
                    break;
          default:
                    *to++ = *p++;
                     p   -= 2;
        }
        p += 2;
      }
      if(to - cmdLine >= 300) break;
    }
    *to = 0;
    
/*    STARTUPINFO sInfo;
    memset(&sInfo, 0, sizeof(STARTUPINFO));
    if(fullScreen) sInfo.dwFlags = STARTF_RUNFULLSCREEN;
    
    PROCESS_INFORMATION procInfo;
    if(!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &sInfo, &procInfo))
    {
      char *msgItems[4];
      msgItems[0] = getMsg(MxTRDWarning);
      msgItems[1] = getMsg(MCanNotExec);
      msgItems[2] = cmdLine;
      msgItems[3] = getMsg(MOk);
      messageBox(FMSG_WARNING | FMSG_DOWN, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    }*/

    return TRUE;
  }
  if(((controlState & PKF_SHIFT) && key == VK_F6) ||
     (!(controlState & PKF_ANY) && key == VK_F4) ||
     ((controlState & PKF_CONTROL) && key == 0x41))
  {
    if(fmt->isProtected(img))
    {
      char *msgItems[4];
      msgItems[0] = getMsg(MWarning);
      msgItems[1] = getMsg(MCanNotEdit);
      msgItems[2] = getMsg(MWriteProtected);
      msgItems[3] = getMsg(MOk);
      messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
      return TRUE;
    }

    HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
    PanelInfo info;
    startupInfo.Control(this,FCTL_GETPANELINFO,&info);
    startupInfo.RestoreScreen(screen);
    
    if(info.ItemsNumber > 0 && info.CurrentItem > 0)
    {
      int fNum = 0;
      char *fName = info.PanelItems[info.CurrentItem].FindData.cFileName;
      if(info.PanelItems[info.CurrentItem].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        for(fNum = 0; fNum < noFolders; ++fNum)
        {
          if(folderMap[fNum] != curFolderNum) continue;
          if(!strcmp(fName, pcFolders[fNum])) break;
        }
        char name[12] = "           ";
        for(int i = 0; i < 11; i++)
          name[i] = folders[fNum][i] ? folders[fNum][i] : ' ';
        
        int isDeleted = (name[0] == 0x01) ? 1 : 0;
        
        InitDialogItem items[]=
        {
          DI_DOUBLEBOX,3,1,34,6,0,0,0,0,(char*)MRenameFolder,
          DI_TEXT,5,2,0,0,0,0,DIF_CENTERGROUP,0,(char*)MFolderName,
          DI_EDIT,13,3,24,0,1,0,0,0,name,
          DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,1,(char*)MOk,
          DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
        };
        FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
        initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
        
        int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                         -1,-1,38,8,
                                         NULL,
                                         dialogItems,
                                         sizeof(dialogItems)/sizeof(dialogItems[0]));
        if(askCode != 4) return TRUE;
        
        char *ptr = dialogItems[2].Data;
        for(int i = 0; i < 11; ++i) folders[fNum][i] = ' ';
        
        for(int i = 0; i < 11; ++i)
        {
          if(*ptr > 0) folders[fNum][i] = *ptr;
          if(*ptr == 0) break;
          ++ptr;
        }
        
        int isDeletedAfter = (folders[fNum][0] == 0x01) ? 1 : 0;
        switch(isDeleted - isDeletedAfter)
        {
          case 1:
                    // восстановили файл
                    noDelFolders--;
                    break;
          case -1:
                    folders[fNum][0] = '?';
                    break;
        }
      }
      else
      {
        for(; fNum < noFiles; ++fNum)
          if(!strcmp(fName, pcFiles[fNum].name)) break;
        
        char name[9] = "        ";
        for(int i = 0; i < 8; i++)
          name[i] = files[fNum].name[i] ? files[fNum].name[i] : ' ';
        
        int isDeleted = (name[0] == 0x01) ? 1 : 0;
        char type[2] = " ";
        type[0] = files[fNum].type ? files[fNum].type : ' ';
        
        char start[6];
        sprintf(start, "%d", files[fNum].start);
        
        InitDialogItem items[]=
        {
          DI_DOUBLEBOX,3,1,34,7,0,0,0,0,(char*)MEditFileInfo,
          DI_TEXT,5,2,0,0,0,0,0,0,(char*)MFileName,
          DI_EDIT,24,2,32,0,1,0,0,0,name,
          DI_TEXT,5,3,0,0,0,0,0,0,(char*)MFileType,
          DI_EDIT,24,3,25,0,0,0,0,0,type,
          DI_TEXT,5,4,0,0,0,0,0,0,(char*)MStartAddress,
          DI_EDIT,24,4,29,0,0,0,0,0,start,
          DI_TEXT,3,5,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_BUTTON,0,6,0,0,0,0,DIF_CENTERGROUP,1,(char*)MOk,
          DI_BUTTON,0,6,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
        };
        FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
        initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
        
        int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                         -1,-1,38,9,
                                         NULL,
                                         dialogItems,
                                         sizeof(dialogItems)/sizeof(dialogItems[0]));
        if(askCode != 8) return TRUE;
        
        char *ptr = dialogItems[2].Data;
        for(int i = 0; i < 8; ++i) files[fNum].name[i] = ' ';
        
        for(int i = 0; i < 8; ++i)
        {
          if(*ptr > 0) files[fNum].name[i] = *ptr;
          if(*ptr == 0) break;
          ++ptr;
        }
        
        int isDeletedAfter = (files[fNum].name[0] == 0x01) ? 1 : 0;
        switch(isDeleted - isDeletedAfter)
        {
          case 1:
                    // восстановили файл
                    noDelFiles--;
                    diskInfo.noDelFiles = noDelFiles;
                    break;
          case -1:
                    files[fNum].name[0] = '?';
                    break;
        }
        
        ptr = dialogItems[4].Data;
        files[fNum].type = 'C';
        if(*ptr > 0) files[fNum].type = *ptr;
        
        int result = str2int(dialogItems[6].Data, 5);
        if(result != -1) files[fNum].start = result;
      }        

      if(!openHostFile()) return TRUE;
      writeInfo();
      closeHostFile();

      startupInfo.Control(this,FCTL_UPDATEPANEL,(void *)1);
      startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
      return TRUE;
    }
  }
  if(key == VK_F2 && !(controlState & PKF_ANY))
  {
    diskMenu();
    return TRUE;
  }
  return FALSE;
}

bool Manager::checkDisk(void)
{
  if(diskInfo.noFiles    != noFiles) return true;
  if(diskInfo.noDelFiles != noDelFiles) return true;

  int totalSecs = 0;
  switch(diskInfo.type)
  {
    case 0x16:
              totalSecs = 159*16;
              break;
    case 0x17:
    case 0x18:
              totalSecs = 79*16;
              break;
    case 0x19:
              totalSecs = 39*16;
              break;
  }

  int trk = 1;
  int sec = 0;
  int noTotalSecs = 0;
  for(int i = 0; i < noFiles; ++i)
  {
    int noSecs = files[i].noSecs;
    noTotalSecs += noSecs;
    
    if(files[i].trk != trk || files[i].sec != sec) return true;
    trk += noSecs / 16;
    sec += noSecs % 16;
    if(sec > 15) { ++trk; sec %= 16; }
  }
  if(diskInfo.firstFreeTrk != trk || diskInfo.firstFreeSec != sec) return true;
  if(noTotalSecs + diskInfo.noFreeSecs != totalSecs) return true;
  
  return false;
}

bool Manager::checkEntry(char num, bool del, bool *checked)
{
  int i;

  for(i = 0; i < noFolders; i++)
    if(folderMap[i]==num)//нашли подкаталог в текущем каталоге
    {
       bool isDel = folders[i][0] == 0x01;
       if(del && !isDel) return true;//в удалёном каталоге неудалённый подкаталог
       checked[i] = true;
       if(checkEntry(i + 1, isDel, checked)) return true;//ошибка в подкаталоге
    }

  return false;
}

bool Manager::checkDS(void)
{
  bool checked[127];
  int i;

  if(noFolders==0) return false; //нечего проверять

  for(i=0; i < noFiles; i++)
    if(fileMap[i] > noFolders) return true;//файл в несуществующем каталоге

//  for(i = 0; i < noFolders; i++)
//    if(folderMap[i] > noFolders) return true;//каталог в несуществующем каталоге

  for(i = 0; i < noFiles; i++)
    if((files[i].name[0] != 0x01) &&
       (folders[fileMap[i] - 1][0] == 0x01)) return true;//файл в удалённом каталоге

  for(i = 0; i < 127; i++)
    checked[i] = false;//нет проверенных каталогов

  if(checkEntry(0, false, checked)) return true;//найдена ошибка

  for(i = 0; i < noFolders; i++)
    if(!checked[i]) return true;//каталог никому не принадлежит

  return false;
}

int Manager::diskMenu(void)
{
  FarMenuItem menuItems[5];
  memset(menuItems, 0, sizeof(menuItems));
  menuItems[0].Selected = TRUE;

  int noMenuItems = 2;

  strcpy(menuItems[0].Text,getMsg(MCheckDisk));
  if(fmt->isProtected(img))
  {
    strcpy(menuItems[1].Text,getMsg(MRemoveProtection));
  }
  else
  {
    strcpy(menuItems[1].Text,getMsg(MProtectDisk));
    strcpy(menuItems[2].Text,getMsg(MEditDiskTitle));
    strcpy(menuItems[3].Text,getMsg(MMoveDisk));
    strcpy(menuItems[4].Text,getMsg(MCleanFreeSpace));
    noMenuItems = 5;
  }
  
  int exitCode = startupInfo.Menu(startupInfo.ModuleNumber,
                                  -1,-1,
                                  0,
                                  FMENU_WRAPMODE,
                                  getMsg(MDiskMenu),
                                  NULL, NULL, NULL, NULL,
                                  menuItems,
                                  noMenuItems);
  if(exitCode == -1) return FALSE;

  switch(exitCode)
  {
    case 0: /* check disk */
              {
                char *msgItems[5];
                msgItems[0] = getMsg(MCheckDisk2);
                msgItems[3] = "\001";
                msgItems[4] = getMsg(MOk);
                char msg1[30];
                if(checkDisk())
                  sprintf(msg1, getMsg(MTestFiles), getMsg(MErrors));
                else
                  sprintf(msg1, getMsg(MTestFiles), getMsg(MNoErrors));
                msgItems[1] = msg1;
                char msg2[30];
                if(!op.useDS)
                  sprintf(msg2, getMsg(MTestDS), getMsg(MDisabled));
                else
                  if(!dsOk)
                    sprintf(msg2, getMsg(MTestDS), getMsg(MAbsent));
                  else
                    if(checkDS())
                      sprintf(msg2, getMsg(MTestDS), getMsg(MErrors));
                    else
                      sprintf(msg2, getMsg(MTestDS), getMsg(MNoErrors));
                msgItems[2] = msg2;
                messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
                break;
              }
    case 1: /* set/remove protection */
              {
                bool ok;
                if(fmt->isProtected(img))
                  ok = fmt->protect(img, false);
                else
                  ok = fmt->protect(img, true);
                if(!ok)
                {
                  char *msgItems[3];
                  msgItems[0] = getMsg(MError);
                  msgItems[1] = getMsg(MCanNotProtect);
                  msgItems[2] = getMsg(MOk);
                  messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
                }
                else
                {
                  startupInfo.Control(this,FCTL_UPDATEPANEL,(void *)1);        
                  startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
                }
                break;
              }
    case 2: /* edit disk title */
              {
                char title[12];
                for(int i = 0; i < 11; ++i)
                  title[i] = (diskInfo.title[i]) ? (diskInfo.title[i]) : ' ';
                title[11] = 0;
                
                InitDialogItem items[] =
                {
                  DI_DOUBLEBOX,3,1,34,6,0,0,0,0,(char*)MEditTitle,
                  DI_TEXT,5,2,0,0,0,0,DIF_CENTERGROUP,0,(char*)MChangeTitle,
                  DI_EDIT,13,3,24,0,1,0,0,0,title,
                  DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
                  DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,1,(char*)MOk,
                  DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
                };
                FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
                initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
                int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                                 -1,-1,38,8,
                                                 NULL,
                                                 dialogItems,
                                                 sizeof(dialogItems)/sizeof(dialogItems[0]));
                if(askCode != 4) return TRUE;

                char* p = dialogItems[2].Data;
                memset(diskInfo.title, ' ', 11);
                for(int i = 0; i < 11; ++i)
                {
                  if(*p > 0) diskInfo.title[i] = *p;
                  if(*p == 0) break;
                  ++p;
                }
                
                if(!openHostFile()) return FALSE;
                writeInfo();
                closeHostFile();

                startupInfo.Control(this,FCTL_UPDATEPANEL,(void *)1);        
                startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
                break;
              }
    case 3: /* move disk */
              {
                if(noDelFiles == 0 && noDelFolders == 0) return TRUE;
                if(!openHostFile()) return FALSE;
                move();
                writeInfo();
                closeHostFile();

                startupInfo.Control(this,FCTL_UPDATEPANEL,(void *)1);        
                startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
                break;
              }
    case 4: /* clean free space */
              {
                BYTE zeroSec[sectorSize];
                memset(zeroSec, 0, sectorSize);
                if(!openHostFile()) return FALSE;
                
                int trk = diskInfo.firstFreeTrk;
                int sec = diskInfo.firstFreeSec;
                for(int i = 0; i < diskInfo.noFreeSecs; ++i)
                {
                  write(trk, sec, zeroSec);
                  if(++sec == 16) { sec = 0; ++trk; }
                  if(trk == 256) break;
                }
                closeHostFile();
                break;
              }
  }
  return TRUE;
}
