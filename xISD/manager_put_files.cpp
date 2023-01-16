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
extern Options           op;

int Manager::putOneFolder(UniHdr* pDir, const WIN32_FIND_DATA& file, const char* fromDir, const char* dirName, int move, Action& action, int opMode)
{
  char cname[8+3+2] = "";
  makeCompatibleFolderName(cname, _N((wchar_t*)file.cFileName).c_str());
  
  int fNum  = findFile(pDir, cname);
  if(!fNum)
  {
    int result = makeFolder(pDir, cname);
    if(result != 1) return result;
  }
  UniHdr curDir[128];
  fNum  = findFile(pDir, cname);
  readFolder(pDir[fNum], curDir);

  char fullName[300];
  makeFullName(fullName, dirName, _N((wchar_t*)file.cFileName).c_str());
  
  char fullFromName[300];
  makeFullName(fullFromName, fromDir, _N((wchar_t*)file.cFileName).c_str());

  char mask[300];
  makeFullName(mask, fullFromName, "*.*");
  
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(_W(mask).c_str(), &data);
  if(h == INVALID_HANDLE_VALUE) return 1;
  int exitCode = 1;
  do
  {
    int result;
    if(!lstrcmp(data.cFileName, L".")) continue;
    if(!lstrcmp(data.cFileName, L"..")) continue;
    if(!reserveSpaceForFiles(curDir, 1))
    {
      exitCode = -1;
      break;
    }
    if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      result = putOneFolder(curDir, data, fullFromName, fullName, move, action, opMode);
    else
      result = putOneFile(curDir, data, fullFromName, fullName, move, action, opMode);

    if(result != 1)
    {
      exitCode = result;
      if(exitCode == -1) break;
    }
   
  } while(FindNextFile(h, &data));
  
  FindClose(h);
  writeFolder(curDir);
  if(move && exitCode == 1)
    if(!RemoveDirectory(_W(fullFromName).c_str())) return 0;
  return exitCode;
}

int Manager::putOneFile(UniHdr* pDir, const WIN32_FIND_DATA& file, const char* fromDir, const char* dirName, int move, Action& action, int opMode)
{
  u16 needBlocks = file.nFileSize / blockSize + (file.nFileSize % blockSize ? 1 : 0);
  int firstFoundBlock = findFreeBlocks(needBlocks);
  
  if(firstFoundBlock == -1)
  {
    char *msgItems[3];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MNoDiskSpace);
    msgItems[2] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return -1;
  }
  char cname[8+3+2];
  makeCompatibleFileName(cname, _N((wchar_t*)file.cFileName).c_str());
  int fNum = findFile(pDir, cname);
  if(fNum)
  {
    // файл или каталог с таким именем уже существует
    if(isDir(pDir[fNum]))  return 0; // это каталог
    if(pDir[fNum].attr == 0xFF && pDir[fNum].file.systemFlag == 0xFF) return 0;
    
    if(action == SKIP_ALL) return 0;
    if(action == ASK_USER)
    {
      char fullName[300];
      makeFullName(fullName, dirName, _N((wchar_t*)file.cFileName).c_str());
      
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
      msgItems[2] = cropLongName(shortName, fullName, 47);
      
      char dest  [50];
      char source[50];
      
      FILETIME lastWriteTime;
      FileTimeToLocalFileTime(&file.ftLastWriteTime, &lastWriteTime);
  
      SYSTEMTIME time;
      FileTimeToSystemTime(&lastWriteTime, &time);
      
      sprintf(source, getMsg(MSrc),
               file.nFileSize,
               time.wDay, time.wMonth, time.wYear,
               time.wHour, time.wMinute, time.wSecond);
      
      SYSTEMTIME st = makeDate(pDir[fNum]);
      sprintf(dest, getMsg(MDest),
               getSize(pDir[fNum]),
               st.wDay, st.wMonth, st.wYear,
               st.wHour, st.wMinute, st.wSecond);
      
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
    // удаляем файл
    Action actProtected = DO_ALL;
    Action actFolder    = DO_ALL;
    deleteOneFile(pDir[fNum], pDir, dirName, OPM_SILENT, actProtected, actFolder);
  }

  char fullName[300];
  makeFullName(fullName, fromDir, _N((wchar_t*)file.cFileName).c_str());
  if(!(opMode & OPM_SILENT))
  {
    char *msgItems[3];
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
    
    char shortName[45];
    msgItems[2] = cropLongName(shortName, fullName, 40);
    messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);
  }
  
  HANDLE hFile = CreateFile(_W(fullName).c_str(),
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
  
  if(hFile == INVALID_HANDLE_VALUE)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotOpen);
    msgItems[2] = (char*)_N((wchar_t*)file.cFileName).c_str();
    msgItems[3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return 0;
  }

  UniHdr newFile;
  memset(&newFile, 0, sizeof(newFile));
  memset(&newFile, ' ', 8+3);
  char *p = cname;
  if(*p == 0) return 0;
  for(int i = 0; i < 8; ++i)
  {
    if(*p == 0 || *p =='.') break;
      newFile.name[i] = *p;
    ++p;
  }
  
  p = (char*)pointToExt(cname);
  
  if(p)
  {
    for(int i = 0; i < 3; ++i)
    {
      if(*p == 0) break;
      newFile.ext[i] = *p;
    ++p;
    }
  }
  
  newFile.attr            = 0x43;
  newFile.file.firstBlock = firstFoundBlock;
  setFileSize(newFile, file.nFileSize);

  SYSTEMTIME time;
  FILETIME lastWriteTime;
  FileTimeToLocalFileTime(&file.ftLastWriteTime, &lastWriteTime);
  FileTimeToSystemTime(&lastWriteTime, &time);
  setDate(newFile, time);

  FILETIME ftCreationTime;
  GetFileTime(hFile, &ftCreationTime, NULL, NULL);
  SYSTEMTIME tmp;
  FileTimeToSystemTime(&ftCreationTime, &tmp);
  if(tmp.wYear == 2000 && tmp.wSecond == 0 && (tmp.wMonth == 1 || tmp.wMonth == 2))
  {
    u16 laddr = tmp.wMinute;
    laddr    += 60*tmp.wHour;
    laddr    += 24*60*(tmp.wDay-1);
    laddr    += 31*24*60*(tmp.wMonth-1);
    newFile.file.loadAddr = laddr;
  }
  u8 buf[blockSize];
  u16 checkSum = 0;
  while(needBlocks--)
  {
    DWORD noBytesRead;
    memset(buf, 0, blockSize);
    ReadFile(hFile, buf, blockSize, &noBytesRead, NULL);
    writeBlock(firstFoundBlock, buf);
    markBlock(firstFoundBlock++);
    for(int i = 0; i < noBytesRead; ++i) checkSum += buf[i];
  }
  if(newFile.ext[0] == 'c' && newFile.ext[1] == 'o' && newFile.ext[2] == 'm')
    newFile.file.crc = 0 - checkSum;
 
  fNum = 1;
  for(; fNum < pDir[0].dir.totalFiles; ++fNum)
    if(!(pDir[fNum].attr & FLAG_EXIST)) break;
  
  memcpy(&pDir[fNum], &newFile, sizeof(UniHdr));
  if(fNum == pDir[0].dir.totalFiles) ++pDir[0].dir.totalFiles;
  ++pDir[0].dir.noFiles;

  CloseHandle(hFile);
  if(move)
    if(!DeleteFile(_W(fullName).c_str())) return 0;
  return 1;
}

const WIN32_FIND_DATA to_win32_find_data(FAR_FIND_DATA data)
{
  WIN32_FIND_DATA result;
  lstrcpy(result.cFileName, _W(data.cFileName).c_str());
  result.dwFileAttributes = data.dwFileAttributes;
  result.ftCreationTime = data.ftCreationTime;
  result.ftLastAccessTime = data.ftLastAccessTime;
  result.ftLastWriteTime = data.ftLastWriteTime;
  result.nFileSize = data.nFileSize;
  result.nPhysicalSize = data.nPhysicalSize;

  return result;
}

int Manager::putFiles(PluginPanelItem *panelItem, int noItems, int move, int opMode)
{
  if(noItems == 0) return 1;
  if(files[0].dir.noFiles + noItems > 127)
  {
    char *msgItems[3];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MMany);
    msgItems[2] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return 0;
  }

  if(!openHostFile()) return 0;
  if(!reserveSpaceForFiles(files, noItems))
  {
    closeHostFile();
    char *msgItems[3];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MNoDiskSpace);
    msgItems[2] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return 0;
  }

  HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
  
  Action action   = ASK_USER;
  int    exitCode = 1;
  for(int iNum = 0; iNum < noItems; ++iNum)
  {
    int result;
    if(panelItem[iNum].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      result = putOneFolder(files, to_win32_find_data(panelItem[iNum].FindData), "", curDir, move, action, opMode);
    else
      result = putOneFile(files, to_win32_find_data(panelItem[iNum].FindData), "", curDir, move, action, opMode);
    
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

  writeFolder(files);
  write_device_sys();
  closeHostFile();
  op.reread = true;
  startupInfo.Control(this,FCTL_UPDATEPANEL, (void *)1);
  op.reread = false;
  return exitCode;
}
