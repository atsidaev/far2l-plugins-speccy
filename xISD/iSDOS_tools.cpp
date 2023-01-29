#include "iSDOS_tools.hpp"
#include "tools.hpp"
#include "far2sdk/farplug-mb.h"
using namespace oldfar;
#include "../shared/widestring.hpp"

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

int PWZ_to_PZ(const wchar_t *src, char *dst, int lendst)
{
	return WINPORT(WideCharToMultiByte)(CP_UTF8,0,(src),-1,(dst),(int)(lendst),nullptr,nullptr);
}

void makeName(const UniHdr& h, u8* buf)
{
  std::vector<WCHAR> wstr1(8 * 2 + 1);
  std::vector<WCHAR> wstr2(3 * 2 + 1);
  MultiByteToWideChar(CP_OEMCP, 0, (char*)h.name, 8, &wstr1[0], wstr1.size() - 1);
  MultiByteToWideChar(CP_OEMCP, 0, (char*)h.ext,  3, &wstr2[0], wstr2.size() - 1);
  std::wstring name(&wstr1[0]);
  std::wstring ext(&wstr2[0]);

  // Trim spaces at the end of the name
  name.erase(name.find_last_not_of(' ') + 1);

  if (ext[0] == ' ')
  {
    // No extension, check if DOS reserved name and fix it if yes
    if(name == _W("com") ||
       name == _W("lpt") ||
       name == _W("prn") ||
       name == _W("con") ||
       name == _W("aux") ||
       name == _W("nul") )
      name += '_';
  }
  else
  {
    name += _W(".");
    name += ext;
  }

  PWZ_to_PZ(name.c_str(), (char*)buf, name.length() * 2 + 1);
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
      u8 name[2 * (8+3+2)];
      makeName(pDir[fNum], name);
      if(!strcmp(fileName, (char*)name)) break;
    }
  }
  if(fNum == pDir[0].dir.totalFiles)
    return 0;
  else
    return fNum;
}
