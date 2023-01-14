#include <windows.h>

#include "manager.hpp"
#include "tools.hpp"
#include "iSDOS.hpp"
#include "iSDOS_tools.hpp"
#include "iterator.hpp"
#include "lang.hpp"

extern Options op;

bool Manager::openHostFile(void) { return filer->openFile(img); }
bool Manager::closeHostFile(void){ return filer->closeFile(img); }

bool Manager::readBlock(u16 num, u8* buf)
{
  bool result = filer->read(img, num, buf);
  if(!result)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotReadBlock);
    msgItems[3] = getMsg(MOk);
    char msg[30];
    wsprintf(msg, getMsg(MBlock), num);
    msgItems[2] = msg;
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
  }
  return result;
}

bool Manager::writeBlock(u16 num, u8* buf)
{
  bool result = filer->write(img, num, buf);
  if(!result)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotWriteBlock);
    msgItems[3] = getMsg(MOk);
    char msg[30];
    wsprintf(msg, getMsg(MBlock), num);
    msgItems[2] = msg;
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
  }
  return result;
}

bool Manager::readFolder(const UniHdr& h, UniHdr* dest)
{
  if(!isDir(h)) return false;

//  ZeroMemory(dest, 128*sizeof(UniHdr));
  
  Iterator cur(filer, img, h), end;

  int i = 0;
  while(cur != end)
  {
    readBlock(*cur, (u8*)dest + i*blockSize);
    ++cur; ++i;
  }
  return true;
}

bool Manager::readFolder(u16 firstBlock, UniHdr* dest)
{
  u8 buf[blockSize];
  readBlock(firstBlock, buf);
  return readFolder(*(UniHdr*)buf, dest);
}

bool Manager::writeFolder(UniHdr* pDir)
{
  if(!isDir(pDir[0])) return false;
  Iterator cur(filer, img, pDir[0]), end;

  int i = 0;
  bool result = true;
  while(cur != end)
  {
    if(!writeBlock(*cur, (u8*)pDir + i*blockSize))
    {
      readBlock(*cur, (u8*)pDir + i*blockSize);
      result = false;
    }
    ++cur; ++i;
  }
  return result;
}

bool Manager::readInfo(void)
{
  u32 noBytesRead;
  
  // проверяем не изменился ли файл на диске
  WIN32_FIND_DATA data;
  HANDLE h = FindFirstFile(hostFileName, &data);
  if(h == INVALID_HANDLE_VALUE) return false;
  FindClose(h);
  
  if(CompareFileTime(&data.ftLastWriteTime, &lastModifed.ftLastWriteTime) != 0 ||
     data.nFileSizeLow != lastModifed.nFileSizeLow || op.reread)
  {
    op.reread = false;
    lastModifed = data;
    if(!filer->reload(img)) return false;
    
    if(!openHostFile()) return false;

    u8 buf[blockSize];

    readBlock(0, buf);
    CopyMemory(&dsk, buf, sizeof(DiskHdr));
    UniHdr temp[128];
    readBlock(files[0].dir.firstBlock, (u8*)temp);
    if((temp[0].attr  & FLAG_EXIST) && compareMemory(files, temp, 8+3))
      readFolder(files[0], files);
    else
    {
      *curDir = 0;
      readFolder(dsk.mainDir1stBlock, files);
    }
    read_device_sys();

    closeHostFile();
  }
  return true;
}

u8 mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

u16 Manager::getNoFreeBlocks(void)
{
  int noBytes      = dsk.noBlocks/8;
  u16 noFreeBlocks = 0;
  u8  n;

  for(int i = 0; i < noBytes; ++i)
  {
    u8 b = device_sys[blockSize+i];
    if(b == 0xFF) continue;
    // считаем количество 1 в байте
    n = (b & 0x55) + ((b & 0xAA) >> 1);
    n = (n & 0x33) + ((n & 0xCC) >> 2);
    n = (n & 0x0f) + ((n & 0xf0) >> 4);
    noFreeBlocks += (8-n);
  }
  u8  b      = device_sys[blockSize+noBytes];
  int noBits = dsk.noBlocks%8;
  for(int j = 0; j < noBits; ++j)
    if(!(b & mask[j])) ++noFreeBlocks;

  return noFreeBlocks;
}

void Manager::read_device_sys(void)
{
  Iterator cur(filer, img, device_sys_hdr), end;
  int i = 0;
  while(cur != end)
  {
    readBlock(*cur, device_sys+i*blockSize);
    ++cur; ++i;
  }
  noFreeBlocks = getNoFreeBlocks();
}

void Manager::write_device_sys(void)
{
  Iterator cur(filer, img, device_sys_hdr), end;
  int i = 0;
  while(cur != end)
  {
    writeBlock(*cur, device_sys+i*blockSize);
    ++cur; ++i;
  }
}

bool Manager::isFreeBlock(u16 n)
{
  if(n > dsk.noBlocks-1) return true;
  int byte = n/8;
  int bit  = n%8;
  return !(device_sys[blockSize+byte] & mask[bit]);
}

bool Manager::markBlock(u16 n)
{
  if(n > dsk.noBlocks-1) return false;
  int byte = n/8;
  int bit  = n%8;
  if(device_sys[blockSize+byte] & mask[bit]) return false;
  device_sys[blockSize+byte] |= mask[bit];
  --noFreeBlocks;
  return true;
}

bool Manager::unmarkBlock(u16 n)
{
  if(n > dsk.noBlocks-1) return false;
  int byte = n/8;
  int bit  = n%8;
  if(!(device_sys[blockSize+byte] & mask[bit])) return false;
  device_sys[blockSize+byte] &= ~mask[bit];
  ++noFreeBlocks;
  return true;
}

bool Manager::findMaxFreeBlocks(u16 n, u16* firstFoundBlock, u16* noFoundBlocks)
{
  *firstFoundBlock = 0;
  *noFoundBlocks   = 0;
  
  u16 noBlocks     = 0;
  u16 firstBlock   = 0;

  int noBytes = dsk.noBlocks/8;
  int i       = 0;
  for(; i < noBytes; ++i)
  {
    if(device_sys[blockSize+i] == 0xFF)
    {
      noBlocks   = 0;
      firstBlock = 0;
      continue;
    }
    for(int bit = 0; bit < 8; ++bit)
    {
      if(device_sys[blockSize+i] & mask[bit])
      {
        noBlocks   = 0;
        firstBlock = 0;
        continue;
      }
      ++noBlocks;
      if(noBlocks == 1) firstBlock = 8*i+bit;

      if(*noFoundBlocks < noBlocks)
      {
        *noFoundBlocks   = noBlocks;
        *firstFoundBlock = firstBlock;
      }
      if(*noFoundBlocks == n) return true;
    }
  }
  
  int noBits = dsk.noBlocks%8;
  for(int bit = 0; bit < noBits; ++bit)
  {
    if(device_sys[blockSize+i] & mask[bit])
    {
      noBlocks   = 0;
      firstBlock = 0;
      continue;
    }
    ++noBlocks;
    if(noBlocks == 1) firstBlock = 8*i+bit;
    
    if(*noFoundBlocks < noBlocks)
    {
      *noFoundBlocks   = noBlocks;
      *firstFoundBlock = firstBlock;
    }
    if(*noFoundBlocks == n) return true;
  }
  return false;
}

int Manager::findFreeBlocks(u16 n)
{
  u16 noBlocks;
  u16 firstBlock;
  if(findMaxFreeBlocks(n, &firstBlock, &noBlocks))
    return firstBlock;
  else
    return -1;
}

bool Manager::reserveSpaceForFiles(UniHdr* pDir, u16 noAddFiles)
{
  if(!isDir(pDir[0])) return false;
  if(pDir[0].dir.noFiles + noAddFiles > 127) return false;

  Iterator cur(filer, img, pDir[0]), end;
  
  int noUsedBlocks = 0;
  while(cur != end)
  {
    ++cur; ++noUsedBlocks;
  }
  u16 n = noUsedBlocks*8 - pDir[0].dir.noFiles - 1;
  if(n >= noAddFiles) return true;

  if(pDir[0].attr & FLAG_SOLID) return false;

  u16 needFiles  = noAddFiles - n;
  u16 needBlocks = needFiles/8 + (needFiles%8 ? 1 : 0);
  if(needBlocks > noFreeBlocks) return false;

  u8 buf[blockSize];
  readBlock(pDir[0].dir.descr1stBlock, buf);

  pDir[0].dir.size += blockSize*needBlocks;
 
  while(needBlocks)
  {
    u16 firstFoundBlock;
    u16 noFoundBlocks;
    findMaxFreeBlocks(needBlocks, &firstFoundBlock, &noFoundBlocks);
    
    *(u16*)(buf+3*buf[0]+1) = firstFoundBlock;
    buf[3*buf[0]+3]         = noFoundBlocks;
    ++buf[0];
    for(int i = 0; i < noFoundBlocks; ++i) markBlock(firstFoundBlock+i);
    needBlocks -= noFoundBlocks;
  }
  writeBlock (pDir[0].dir.descr1stBlock, buf);
  writeFolder(pDir);
  write_device_sys();  

  return true;
}
