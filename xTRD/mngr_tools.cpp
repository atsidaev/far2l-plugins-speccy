#include <windows.h>

#include "manager.hpp"
#include "detector.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"
#include "widestring.hpp"

extern Options           op;
extern Detector*         detector;

char dsSignature[] = { 'D', 'i', 'r', 'S', 'y', 's', '1', '0', '0' };

bool Manager::openHostFile(void) { return fmt->open(img); }
void Manager::closeHostFile(void){        fmt->close(img); }

bool Manager::read(int trk, int sec, BYTE* buf)
{
  bool result = fmt->read(img, trk, sec, buf);
  if(!result)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotReadSec);
    msgItems[3] = getMsg(MOk);
    char msg[30];
    sprintf(msg, getMsg(MTrkSec), trk, sec);
    msgItems[2] = msg;
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
  }
  return result;
}

bool Manager::write(int trk, int sec, BYTE* buf)
{
  bool result = fmt->write(img, trk, sec, buf);
  if(!result)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotWriteSec);
    msgItems[3] = getMsg(MOk);
    char msg[30];
    sprintf(msg, getMsg(MTrkSec), trk, sec);
    msgItems[2] = msg;
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
  }
  return result;
}

bool Manager::readInfo(void)
{
  // проверяем не изменился ли файл на диске
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(_W(hostFileName).c_str(), &data);
  if(h == INVALID_HANDLE_VALUE) return false;
  FindClose(h);
  if(!op.useDS)
  {
    curFolderNum = 0;
    *curFolder   = 0;
  }
  
  if(CompareFileTime(&data.ftLastWriteTime, &lastModifed.ftLastWriteTime) != 0 ||
     data.nFileSize != lastModifed.nFileSize || op.reread)
  {
    op.reread    = false;
    lastModifed  = data;
    noFiles      = 0;
    noDelFiles   = 0;
    
    if(!fmt->reload(img)) return false;
    if(!openHostFile()) return false;
    
    // читаем 0-ую дорожку
    for(int i = 0; i < 16; ++i) read(0, i, zeroTrk+i*sectorSize);

    // получаем данные о файлах
    BYTE* ptr = zeroTrk;
    for(; noFiles < 142; ++noFiles)
    {
      if(*ptr == 0x00) break;
      if(*ptr == 0x01) ++noDelFiles;

      memcpy(&files[noFiles], ptr, sizeof(FileHdr));
      // стираем каталог, чтобы в дальнейшем
      // проще было делать MOVE
      *ptr = 0x00; 
      ptr += sizeof(FileHdr);
    }
    
    memcpy(&diskInfo, zeroTrk+8*sectorSize+0xE1, sizeof(DiskHdr));
    
    // обрабатываем DirSys
    BYTE folderName[11];
    if(curFolderNum != 0)
      memcpy(folderName, folders[curFolderNum-1], 11);

    noFolders    = 0;
    noDelFolders = 0;

    dsOk  = false;
    ptr   = zeroTrk+9*sectorSize;
    if(!memcmp(ptr+2, dsSignature, sizeof(dsSignature)))
    {
      dsCRC = *(WORD*)ptr;
      memcpy(fileMap,   ptr+2+sizeof(dsSignature),     128);
      memcpy(folderMap, ptr+2+sizeof(dsSignature)+128, 127);
      ptr = zeroTrk+10*sectorSize+11;
      for(; noFolders < 127; ++noFolders)
      {
        if(*ptr == 0x00) break;
        if(*ptr == 0x01) ++noDelFolders;

        memcpy(folders[noFolders], ptr, 11);
        ptr += 11;
      }
      dsOk = (dsCRC == calcDScrc());
      // стираем каталог, чтобы в дальнейшем
      // проще было делать MOVE
      ptr = zeroTrk+10*sectorSize+11;
      for(int i = 0; i < noFolders; ++i) *(ptr + 11*i) = 0;
      if(!dsOk)
      {
        curFolderNum = 0;
        *curFolder   = 0;
      }
      if(curFolderNum != 0)
      {
        int i = curFolderNum > noFolders ? noFolders : curFolderNum;
        for(; i > 0; --i)
          if(!memcmp(folders[i-1], folderName, 11)) break;
        if(i == 0)
        {
          curFolderNum = 0;
          *curFolder   = 0;
        }
        else
          curFolderNum = i;
      }
    }
    makePCNames();
    closeHostFile();
  }
  return true;
}

void Manager::writeInfo(void)
{
  memcpy(zeroTrk, files, noFiles*sizeof(FileHdr));
  memcpy(zeroTrk+8*sectorSize+0xE1, &diskInfo, sizeof(DiskHdr));
  for(int i = 0; i < 9; ++i) write(0, i, zeroTrk+i*sectorSize);

  BYTE* dsPtr = zeroTrk+9*sectorSize;
  if(dsOk)
  {
    memcpy(dsPtr+2,      dsSignature, sizeof(dsSignature));

    memcpy(dsPtr+11,     fileMap,     128);
    memcpy(dsPtr+11+128, folderMap,   127);
    memcpy(dsPtr+11+256, folders,     11*noFolders);

    dsPtr[11+255]              = 0;
    dsPtr[11+256+11*noFolders] = 0;

    dsCRC = calcDScrc();
    memcpy(dsPtr, &dsCRC, 2);
    for(int i = 9; i < 16; ++i) write(0, i, zeroTrk+i*sectorSize);
  }
}

void Manager::makePCNames(void)
{
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
    BYTE* from    = (BYTE*)files  [fNum].name;
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
      if(!memcmpi((const char*)to, "com", 3) ||
         !memcmpi((const char*)to, "lpt", 3) ||
         !memcmpi((const char*)to, "prn", 3) ||
         !memcmpi((const char*)to, "con", 3) ||
         !memcmpi((const char*)to, "aux", 3) ||
         !memcmpi((const char*)to, "nul", 3)) to[noChars++] = '_';
    }

    BYTE dotPos = noChars;
    to[noChars++] = '.';
    to[noChars++] = '$';
    if(isValidChar(files[fNum].type)) to[noChars++] = files[fNum].type;
    to[noChars] = 0;
    
    BYTE typeNum = 0xFF;
    if(op.detectFormat)
    {
      BYTE secs[16*sectorSize];
      memset(secs, 0, 16*sectorSize);

      int noSecs = files[fNum].noSecs;
      if(noSecs > 16) noSecs = 16;
      int trk = files[fNum].trk;
      int sec = files[fNum].sec;
      int s = 0;

      for(; s < noSecs; ++s)
      {
        if(!fmt->read(img, trk, sec, secs+s*sectorSize)) break;
        ++sec;
        if(sec == 16) { ++trk; sec = 0; }
      }
      typeNum = detector->detect(files[fNum], secs, sectorSize*s, pcFiles[fNum].comment);
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
       !memcmp(from, "********ZIP", 11) &&
       !memcmp(&files[fNum-1].type, "ZIP", 3))
    {
      if(memcmp(files[fNum-1].name, "********", 8))
        strcpy((char*)to, pcFiles[fNum-1].name);
      else
        strncpy((char*)to, pcFiles[fNum-1].name, strlen(pcFiles[fNum-1].name));
    }
    // обрабатываем повторяющиеся имена
    int i = fNum;
    while(i-- > 0)
    {
      int len = strlen((const char*)to);
      if(!memcmpi((const char*)to, pcFiles[i].name, len))
      {
        BYTE ch = pcFiles[i].name[len];
        ch = (ch != 0) ? ch+1 : '0';
        
        // вот такая кривая обработка :
        if(ch == '9'+1) ch = 'A'; 
        if(ch == 'Z'+1) ch = 'a';
        if(ch == 'z'+1) ch = '0';
        
        to[len] = ch; to[len+1] = 0;
        break;
      }
    }
  }
  // обрабатываем имена каталогов
  for(int fNum = 0; fNum < noFolders; ++fNum)
  {
    char* ptr = pcFolders[fNum];
    memcpy(ptr, folders[fNum], 8);
    ptr[8] = 0;
    trim(ptr);
    for(int i = 0; i < strlen(ptr); ++i) if(!isValidFolderChar(ptr[i])) ptr[i] = '_';
    
    char ext[4];
    memcpy(ext, folders[fNum]+8, 3);
    ext[3] = 0;
    trim(ext);
    if(ext[0] != 0)
    {
      for(int i = 0; i < strlen(ext); ++i) if(!isValidFolderChar(ext[i])) ext[i] = '_';
      strcat(ptr, ".");
      strcat(ptr, ext);
    }
    int i = fNum;
    while(i-- > 0)
    {
      if(folderMap[i] != folderMap[fNum]) continue;
      
      int len = strlen(ptr);
      if(!memcmpi(ptr, pcFolders[i], len))
      {
        if(strlen(pcFolders[i]) == len)
        {
          strcat(ptr, ".0");
          break;
        }
        char ch = pcFolders[i][len+1];
        if(pcFolders[i][len] == '.' && isEnAlphaNum(ch))
        {
          strcat(ptr, ".0");
          ++ch;
          if(ch == '9'+1) ch = 'A'; 
          if(ch == 'Z'+1) ch = 'a';
          if(ch == 'z'+1) ch = '0';
          pcFolders[fNum][len+1] = ch;
          break; 
        }
      }
    }
  }
}

WORD Manager::calcDScrc(void)
{
  return crc(zeroTrk+9*sectorSize+2, 256+9+11*noFolders);
}
