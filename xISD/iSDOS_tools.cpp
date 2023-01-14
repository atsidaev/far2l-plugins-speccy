#include "iSDOS_tools.hpp"
#include "tools.hpp"

bool isDir(const UniHdr& h)
{
  if(h.attr == 0xFF && h.file.systemFlag == 0xFF) return false;
  if(h.attr & FLAG_DIR)
    return true;
  else
    return false;
}

u32 getSize(const UniHdr& h)
{
  if(isDir(h))
    return h.dir.size;
  else
    return 256*(256*h.file.size[2] + h.file.size[1]) + h.file.size[0];
}

void setFileSize(UniHdr& h, int size)
{
  if(isDir(h)) return;
  h.file.size[0] = size%256;
  size /= 256;
  h.file.size[1] = size%256;
  size /= 256;
  h.file.size[2] = size;
}


void makeName(const UniHdr& h, u8* buf)
{
  int i = 0;
  for(; i < 8; ++i)
  {
    if(h.name[i] == ' ') break;
    buf[i] = h.name[i];
  }

  if(i == 3 || i == 4)
  {
    if(strncmp((const char*)buf, "com", 3) == 0 ||
       strncmp((const char*)buf, "lpt", 3) == 0 ||
       strncmp((const char*)buf, "prn", 3) == 0 ||
       strncmp((const char*)buf, "con", 3) == 0 ||
       strncmp((const char*)buf, "aux", 3) == 0 ||
       strncmp((const char*)buf, "nul", 3) == 0) buf[i++] = '_';
  }

  if(h.ext[0] != ' ')
  {
    buf[i++] = '.';
    for(int j = 0; j < 3; ++j)
    {
      if(h.ext[j] == ' ') break;
      buf[i++] = h.ext[j];
    }
  }
  buf[i] = 0;
}


SYSTEMTIME makeDate(const UniHdr& h)
{
  SYSTEMTIME t;
  memset(&t, 0, sizeof(SYSTEMTIME));
  
  t.wDay   = h.date & 0x1F;
  t.wMonth = (h.date>>5) & 0x0F;
  t.wYear  = 1980 + ((h.date>>9) & 0x7F);
  if(!isDir(h))
  {
    t.wHour   = (h.file.time>>11) & 0x1F;
    t.wMinute = (h.file.time>>5) & 0x3F;
    t.wSecond = 2*(h.file.time & 0x1F);
  }
  return t;
}

void setDate(UniHdr& h, const SYSTEMTIME& t)
{
  h.date   = t.wDay & 0x1F;
  h.date  |= (t.wMonth & 0x0F)<<5;
  h.date  |= ((t.wYear-1980) & 0x7F)<<9;
  
  if(!isDir(h))
  {
    h.file.time  = (t.wSecond/2) & 0x1F;
    h.file.time |= (t.wMinute    & 0x3F)<<5;
    h.file.time |= (t.wHour      & 0x1F)<<11;
  }
}


u16 findFile(const UniHdr* pDir, const char* fileName)
{
  int fNum = 1;
  for(; fNum < pDir[0].dir.totalFiles; ++fNum)
  {
    if(pDir[fNum].attr & FLAG_EXIST)
    {
      u8 name[8+3+2];
      makeName(pDir[fNum], name);
      if(!strcmp(fileName, (char*)name)) break;
    }
  }
  if(fNum == pDir[0].dir.totalFiles)
    return 0;
  else
    return fNum;
}
