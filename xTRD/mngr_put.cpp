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
  enum Action { ASK_USER, WRITE_ALL, SKIP_ALL };
  Action userAction;

  bool   keepSilence;
}

enum FileType { FMT_SCL, FMT_HOBETA, FMT_PLAIN };

FileType detectFileType(HANDLE file)
{
  DWORD fileSize = GetFileSize(file, NULL);
  
  DWORD noBytesRead;
  BYTE  buf[sizeof(HoHdr)];
  ReadFile(file, buf, sizeof(HoHdr), &noBytesRead, 0);
  SetFilePointer(file, 0, NULL, FILE_BEGIN);

  // проверяем уж не HoBeta ли это
  HoHdr* hdr = (HoHdr*)buf;
  if(hdr->checkSum == calculateCheckSum(*hdr) &&
     fileSize >= sectorSize*hdr->noSecs + sizeof(HoHdr)) return FMT_HOBETA;
  
  // проверяем уж не SCL ли это

  // проверка длины файла написана таким образом,
  // чтобы учесть некорректные scl-файлы создаваемые
  // старыми версиями SN
  
  char signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R' };
  BYTE no_files = buf[sizeof(signature)];
  if(fileSize < no_files*(sectorSize+sizeof(SCLHdr))+8+1+4 ||
     memcmp(buf, signature, sizeof(signature))) return FMT_PLAIN;
  
  DWORD totalSecs = 0;
  SetFilePointer(file, 8+1, NULL, FILE_BEGIN);
  for(int i = 0; i < no_files; ++i)
  {
    SCLHdr fHdr;
    ReadFile(file, &fHdr, sizeof(SCLHdr), &noBytesRead, 0);
    totalSecs += fHdr.noSecs;
  }
  SetFilePointer(file, 0, NULL, FILE_BEGIN);
  
  if(fileSize < totalSecs*sectorSize+no_files*sizeof(SCLHdr)+8+1+4)
    return FMT_PLAIN;
  else
    return FMT_SCL;
}

bool Manager::isDiskFull(int noAddFiles, int noSecs)
{
  if(noFiles + noAddFiles > 128)
  {
    char *msgItems[3];
    msgItems[0] = getMsg(MDiskFull);
    msgItems[1] = getMsg(MTooManyFiles);
    msgItems[2] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return true;
  }
  if(noSecs > diskInfo.noFreeSecs)
  {
    char str1[80];
    char str2[80];
    sprintf(str1, getMsg(MFileSize), noSecs);
    sprintf(str2, getMsg(MFree), diskInfo.noFreeSecs);
    char *msgItems[4];
    msgItems[0] = getMsg(MDiskFull);
    msgItems[1] = str1;
    msgItems[2] = str2;
    msgItems[3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return true;
  }
  return false;
}

ExitCode Manager::writeFile(const SCLHdr& h, HANDLE file, BYTE folderNum)
{
  if(isDiskFull(1, h.noSecs)) return CANCEL;
  
  FileHdr hdr;
  memcpy(&hdr, &h, sizeof(SCLHdr));
  hdr.trk = diskInfo.firstFreeTrk;
  hdr.sec = diskInfo.firstFreeSec;
  
  int fNum;
  for(fNum = 0; fNum < noFiles; ++fNum)
  {
    if(dsOk && fileMap[fNum] != folderNum) continue;
    if(!memcmp(files[fNum].name, hdr.name, 9)) break;
  }
  if(fNum != noFiles)
  {
    if(userAction == SKIP_ALL) return SKIP;
    if(userAction == ASK_USER)
    {
      char *msgItems[13];
      msgItems[0] = getMsg(MWarning);
      msgItems[1] = getMsg(MAlreadyExists);
      msgItems[3] = "\x001";
      msgItems[4] = getMsg(MLine);
      msgItems[7] = "\x001";
      msgItems[8] = getMsg(MWrite);
      msgItems[9] = getMsg(MAll);
      msgItems[10] = getMsg(MSkip);
      msgItems[11] = getMsg(MSkipAll);
      msgItems[12] = getMsg(MCancel);
      
      BYTE name[13] = "            ";
      makeTrDosName((char*)name, hdr, 12);
      msgItems[2] = (char*)name;
      
      char source[50];
      char dest[50];
      sprintf(source, getMsg(MSource),
               (int)hdr.size,
               hdr.noSecs);
      sprintf(dest,   getMsg(MDestination),
               (int)files[fNum].size,
               files[fNum].noSecs);
      
      msgItems[5] = source;
      msgItems[6] = dest;
      
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
                  {
                    // необходимо для корректной обработки SCL и PLAIN файлов
                    SetFilePointer(file, hdr.noSecs*sectorSize, NULL, FILE_CURRENT);
                    return SKIP;
                  }
        case 1:
                  userAction = WRITE_ALL;
      }
    }
  }
  // копируем тело файла
  int trk = diskInfo.firstFreeTrk;
  int sec = diskInfo.firstFreeSec;

  if(hdr.noSecs)
  {
    WORD noSecs = hdr.noSecs;
    while(noSecs--)
    {
      BYTE sector[sectorSize];
      memset(&sector, 0, sizeof(sector));
      DWORD noBytesRead;
      ReadFile(file, sector, sectorSize, &noBytesRead, NULL);
      write(trk, sec, sector);
      if(++sec == 16) { ++trk; sec = 0; }
    }
  }
  diskInfo.firstFreeTrk = trk;
  diskInfo.firstFreeSec = sec;

  if(dsOk) fileMap[noFiles] = folderNum;
  
  files[noFiles++] = hdr;
  if(hdr.name[0] == 0x01) ++noDelFiles;
  diskInfo.noFreeSecs -= hdr.noSecs;
  
  return OK;
}

ExitCode Manager::putFile(char* fileName, BYTE folderNum, bool move)
{
  char *msgItems[3];
  msgItems[0] = getMsg(MCopyingFile);
  msgItems[1] = getMsg(MCopyTheFile);

  char shortName[50];
  msgItems[2] = truncPathStr(shortName, fileName, 30);
  
  if(!keepSilence)
    messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);

  HANDLE file = CreateFile(_W(fileName).c_str(),
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);

  if(file == INVALID_HANDLE_VALUE)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotOpen);
    msgItems[2] = fileName;
    msgItems[3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return CANCEL;
  }

  DWORD noBytesRead;
  
  ExitCode exitCode = OK;
  FileType type     = detectFileType(file);
  
  if(type == FMT_HOBETA)
  {
    HoHdr hoHdr;
    ReadFile(file, &hoHdr, sizeof(HoHdr), &noBytesRead, 0);

    SCLHdr h;
    memcpy(&h, &hoHdr, sizeof(SCLHdr));
    h.noSecs = hoHdr.noSecs;
    exitCode = writeFile(h, file, folderNum);
  }
  if(type == FMT_SCL)
  {
    SetFilePointer(file, 8, NULL, FILE_BEGIN);
    BYTE no_files;
    
    ReadFile(file, &no_files, 1, &noBytesRead, 0);

    SCLHdr* hdrs = new SCLHdr[no_files];
    ReadFile(file, hdrs, no_files*sizeof(SCLHdr), &noBytesRead, NULL);

    for(int fNum = 0; fNum < no_files; ++fNum)
    {
      ExitCode code = writeFile(hdrs[fNum], file, folderNum);

      if(code != OK) exitCode = code;
      if(code == CANCEL) break;
    }
    delete[] hdrs;
  }
  if(type == FMT_PLAIN)
  {
    DWORD fileSize = GetFileSize(file, NULL);;
    int sizeInSec  = fileSize/sectorSize  + (fileSize%sectorSize ? 1 : 0);
    int no_files   = sizeInSec/255 + !!(sizeInSec%255);
    if(!no_files) no_files = 1;
    
    char name[11] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'C', 0, 0};
    if(no_files > 1) memset(name, '_', 8);
    
    make8x3name(pointToName(fileName), name);
    for(int fNum = 0; fNum < no_files; ++fNum)
    {
      SCLHdr hdr;
      
      if(no_files > 1)
      {
        char num[5];
        sprintf(num, "_%03d", fNum);
        memcpy(name + 4, num, 4);
      }
      
      memcpy(hdr.name, name, 8);
      memcpy(&hdr.type, name+8,  3);
      if(sizeInSec > 255)
      {
        hdr.noSecs = 255;
        hdr.size   = 255 * sectorSize;
        
        sizeInSec -= 255;
        fileSize  -= 255 * sectorSize;
      }
      else
      {
        // последний кусок
        hdr.noSecs = sizeInSec;
        hdr.size   = fileSize;
      }
      ExitCode code = writeFile(hdr, file, folderNum);

      if(code != OK) exitCode = code;
      if(code == CANCEL) break;
    }
  }
  
  CloseHandle(file);
  if(exitCode == OK && move) DeleteFile(_W(fileName).c_str());
  return exitCode;
}

bool Manager::makeFolder(char* folderName, BYTE folderNum)
{
  if(!op.useDS) return false;
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
    return false;
  }

  memset(folders[noFolders], ' ', 11);
  
  make8x3name(folderName, folders[noFolders]);
  folderMap[noFolders]    = folderNum;
  // по делу надо бы нормально сформировать имя
  pcFolders[noFolders][0] = 0; 
  if(folders[noFolders][0] == 0x01) ++noDelFolders;
  ++noFolders;
  return true;
}

ExitCode Manager::putFolder(char* folderName, BYTE folderNum, bool move)
{
  int fNum = 0;
  char* name = pointToName(folderName);
  for(; fNum < noFolders; ++fNum)
  {
    if(folderMap[fNum] != folderNum) continue;
    if(!strcmp(pcFolders[fNum], name)) break;
  }
  if(fNum == noFolders)
  {
    // каталога с таким именем не существует
    if(!makeFolder(name, folderNum)) return CANCEL;
  }

  char mask[300];
  strcpy(mask, folderName);
  addEndSlash(mask);
  strcat(mask, "*.*");

  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(_W(mask).c_str(), &data);
  if(h == INVALID_HANDLE_VALUE) return OK;

  ExitCode exitCode = OK;
  do
  {
    if(!strcmp(_N(data.cFileName).c_str(), ".")) continue;
    if(!strcmp(_N(data.cFileName).c_str(), "..")) continue;

    ExitCode code;
    
    char fullName[300];
    strcpy(fullName, folderName);
    addEndSlash(fullName);
    strcat(fullName, _N(data.cFileName).c_str());
    
    if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      code = putFolder(fullName, fNum+1, move);
    else
      code = putFile(fullName, fNum+1, move);

    if(code != OK) exitCode = code;
    if(code == CANCEL) break;
    
  }while(FindNextFile(h, &data));
  FindClose(h);

  if(move && exitCode == OK)
    if(!RemoveDirectory(_W(folderName).c_str())) return SKIP;
  return exitCode;
}

int Manager::putFiles(PluginPanelItem *panelItem, int noItems, int move, int opMode)
{
  if(noItems == 0) return 1;
  if(fmt->isProtected(img))
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MWarning);
    msgItems[1] = getMsg(MCanNotCopy);
    msgItems[2] = getMsg(MWriteProtected);
    msgItems[3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return FALSE;
  }
  
  if(!openHostFile()) return 0;
  
  int returnCode   = 1;
  userAction       = ASK_USER;
  keepSilence      = opMode & OPM_SILENT;
  
  HANDLE file;
  HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
  for(int iNum = 0; iNum < noItems; ++iNum)
  {
    ExitCode exitCode;

    if(panelItem[iNum].FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      exitCode = putFolder(panelItem[iNum].FindData.cFileName, curFolderNum, move);
    else
      exitCode = putFile(panelItem[iNum].FindData.cFileName, curFolderNum, move);
    
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
  diskInfo.noFiles     = noFiles;
  diskInfo.noDelFiles  = noDelFiles;

  writeInfo();
  closeHostFile();
  op.reread = true;
  startupInfo.Control(this,FCTL_UPDATEPANEL, (void *)1);
  op.reread = false;
  return returnCode;
}
