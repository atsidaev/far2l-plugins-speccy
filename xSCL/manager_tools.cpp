#include <windows.h>
#include <far2sdk/farplug-mb.h>
using namespace oldfar;

#include "manager.hpp"
#include "detector.hpp"
#include "types.hpp"
#include "tools.hpp"

#include "debug.hpp"
#include "../shared/widestring.hpp"

extern Detector* detector;
extern Options op;

bool Manager::openHostFile(void)
{
  isReadOnly = false;
  DWORD mode = GENERIC_READ | GENERIC_WRITE;

  DWORD attr = GetFileAttributes(_W(hostFileName).c_str());
  if(attr & FILE_ATTRIBUTE_READONLY)
  {
    mode = GENERIC_READ;
    isReadOnly = true;
  }

  hostFile = CreateFile(_W(hostFileName).c_str(),
                        mode,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);
  return (hostFile != INVALID_HANDLE_VALUE);
}

void Manager::closeHostFile(void)
{
  CloseHandle(hostFile);
}

void Manager::makePCNames(void)
{
  DWORD firstFileOffset = noFiles*sizeof(FileHdr) + 8 + 1;
  for(int fNum = 0; fNum < noFiles; ++fNum)
  {
    int first = 0, last = 7;
    
    while(first <= last)
    {
      if(isValidChar(files[fNum].name[first])) break;
      ++first;
    }
    while(last >= first)
    {
      if(isValidChar(files[fNum].name[last])) break;
      --last;
    }
    BYTE  noChars = 0;
    BYTE* from    = (BYTE*)files[fNum].name;
    BYTE* to      = (BYTE*)pcFiles[fNum].name;
    
    if(first > last)
    {
      memset(to, '_', 8);
      noChars = 8;
    }
    else
    {
      while(first <= last)
      {
        BYTE ch = from[first++];
        to[noChars++] = isValidChar(ch) ? ch : '_';
      }
    }

    // обрабатываем специальные имена устройств
    if(noChars == 3 || noChars == 4)
    {
      if(compareMemoryIgnoreCase(to, (BYTE*)"com", 3) ||
         compareMemoryIgnoreCase(to, (BYTE*)"lpt", 3) ||
         compareMemoryIgnoreCase(to, (BYTE*)"prn", 3) ||
         compareMemoryIgnoreCase(to, (BYTE*)"con", 3) ||
         compareMemoryIgnoreCase(to, (BYTE*)"aux", 3) ||
         compareMemoryIgnoreCase(to, (BYTE*)"nul", 3)) to[noChars++] = '_';
    }
    
    BYTE dotPos = noChars;
    to[noChars++] = '.';
    to[noChars++] = '$';
    if(isValidChar(files[fNum].type)) to[noChars++] = files[fNum].type;
    to[noChars] = 0;
    
    BYTE typeNum = 0xFF;
    if(op.detectFormat)
    {
      BYTE secs[16*256];
      memset(secs, 0, 16*256);

      int noSecs = files[fNum].noSecs;
      if(noSecs > 16) noSecs = 16;
      SetFilePointer(hostFile,
                   sectorSize*pcFiles[fNum].noSecsBefore+firstFileOffset,
                   NULL,
                   FILE_BEGIN);
    
      DWORD noBytesRead;
      ReadFile(hostFile, secs, noSecs*256, &noBytesRead, NULL);
      
      typeNum = detector->detect(files[fNum], secs, noSecs*256, pcFiles[fNum].comment);
    }
    pcFiles[fNum].type = typeNum;

    pcFiles[fNum].skipHeader = false;

    if(typeNum != 0xFF)
    {
      detector->specialChar(typeNum, (char*)(to+dotPos+1));
      detector->getType(typeNum, (char*)(to+dotPos+2));
      pcFiles[fNum].skipHeader = detector->getSkipHeader(typeNum);
    }
    // обрабатываем многотомные zxzip архивы
    if(fNum != 0 &&
       compareMemory(from, (BYTE*)"********ZIP", 11) &&
       compareMemory((BYTE*)&files[fNum-1].type, (BYTE*)"ZIP", 3))
    {
      if(!compareMemory((BYTE*)files[fNum-1].name, (BYTE*)"********", 8))
        strcpy((char*)to, pcFiles[fNum-1].name);
      else
        strncpy((char*)to, pcFiles[fNum-1].name, strlen(pcFiles[fNum-1].name));
    }
    // обрабатываем повторяющиеся имена
    int i = fNum;
    while(i-- > 0)
    {
      int len = strlen((char*)to);
      if(compareMemoryIgnoreCase(to, (BYTE*)pcFiles[i].name, len))
      {
        BYTE ch = pcFiles[i].name[len];
        ch = (ch != 0) ? ch+1 : '0';
        
        // вот такая кривая обработка :(
        if(ch == '9'+1) ch = 'A'; 
        if(ch == 'Z'+1) ch = 'a';
        if(ch == 'z'+1) ch = '0';
        
        to[len] = ch; to[len+1] = 0;
        break;
      }
    }
    pcFiles[fNum].deleted = false;
  }
}

bool Manager::readInfo(void)
{
  DWORD noBytesRead;
  
  // проверяем не изменился ли файл на диске
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(_W(hostFileName).c_str(), &data);
  if(h == INVALID_HANDLE_VALUE) return false;
  FindClose(h);
  
  if(CompareFileTime(&data.ftLastWriteTime, &lastModifed.ftLastWriteTime) != 0 ||
     data.nFileSize != lastModifed.nFileSize)
  {
    lastModifed = data;
    if(!openHostFile()) return false;

    SetFilePointer(hostFile, 8, NULL, FILE_BEGIN);
    ReadFile(hostFile, &noFiles, 1, &noBytesRead, NULL);
    if(noFiles == 0)
    {
      closeHostFile();
      return true;
    }

    DWORD totalSecs = 0;
    for(int i = 0; i < noFiles; ++i)
    {
      pcFiles[i].noSecsBefore = totalSecs;

      BYTE* p = (BYTE*)&files[i];
      ReadFile(hostFile, p, sizeof(FileHdr), &noBytesRead, NULL);
      totalSecs += files[i].noSecs;
    }
    makePCNames();
    closeHostFile();
  }
  return true;
}

DWORD Manager::copyFile(int fileNum, HANDLE file, bool skipGarbage)
{
  if(!files[fileNum].noSecs) return 0;
  DWORD firstFileOffset = noFiles*sizeof(FileHdr) + 8 + 1;
  SetFilePointer(hostFile,
                 sectorSize*pcFiles[fileNum].noSecsBefore+firstFileOffset,
                 NULL,
                 FILE_BEGIN);
  SetFilePointer(file, 0, NULL, FILE_END);
  
  DWORD noBytesWritten, noBytesRead;
  BYTE  sector[sectorSize];
  DWORD checkSum = 0;
  for(int s = 0; s < files[fileNum].noSecs-1; ++s)
  {
    ReadFile (hostFile, sector, sectorSize, &noBytesRead, NULL);
    WriteFile(file,     sector, sectorSize, &noBytesWritten, NULL);
    checkSum += calculateCheckSum(sector, sectorSize);
  }
  // обрабатываем последний сектор хитрым образом
  ReadFile(hostFile, sector, sectorSize, &noBytesRead, NULL);
  checkSum += calculateCheckSum(sector, sectorSize);
  int noBytesToWrite = sectorSize;
  int size           = files[fileNum].size;
  int noSecs2        = size/sectorSize + (size%sectorSize ? 1 : 0);
  if(skipGarbage && files[fileNum].noSecs == noSecs2 && size%sectorSize)
    noBytesToWrite = size % sectorSize;
  WriteFile(file, sector, noBytesToWrite, &noBytesWritten, NULL);
  return checkSum;
}

DWORD Manager::writeSCLHeader(HANDLE file, BYTE no_files)
{
  BYTE  signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R' };
  DWORD noBytesWritten;
  
  WriteFile(file, signature, sizeof(signature), &noBytesWritten, NULL);
  WriteFile(file, &no_files, 1, &noBytesWritten, NULL);
  
  // резервируем место для каталога
  SetFilePointer(file, no_files*sizeof(FileHdr)+8+1, NULL, FILE_BEGIN);
  SetEndOfFile(file);
  
  return (0x255 + no_files);
}
