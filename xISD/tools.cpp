#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "tools.hpp"
#include "../shared/widestring.hpp"

extern PluginStartupInfo startupInfo;

const char* pointToName(const char* path)
{
  const char *namePtr = path;
  while(*path)
  {
    if(*path == '\\' || *path == '/' || *path==':') namePtr = path+1;
    path++;
  }
  return namePtr;
}

const char* pointToExt(const char* path)
{
  int i = strlen(path);
  const char *ptr = path + i;
  while(*ptr != '.' && i != 0) { ptr--; i--; }
  if(i)
    return (ptr+1);
  else
    return 0;
}


void addEndSlash(char *path)
{
  if(!path) return;
  int length = strlen(path);
  if(length != 0 && path[length-1] == ':') return;
  if(length != 0 && path[length-1] != '/') strcat(path,"/");
}

char* getMsg(int msgId)
{
  return((char*)startupInfo.GetMsg(startupInfo.ModuleNumber, msgId));
}

void initDialogItems(InitDialogItem *init, FarDialogItem* item, int noItems)
{
  for(int i = 0; i < noItems; i++)
  {
    item[i].Type          = init[i].Type;
    item[i].X1            = init[i].X1;
    item[i].Y1            = init[i].Y1;
    item[i].X2            = init[i].X2;
    item[i].Y2            = init[i].Y2;
    item[i].Focus         = init[i].Focus;
    item[i].Selected      = init[i].Selected;
    item[i].Flags         = init[i].Flags;
    item[i].DefaultButton = init[i].DefaultButton;
    if((unsigned long)init[i].Data < 2000)
      strcpy(item[i].Data, getMsg((unsigned long)init[i].Data));
    else
      strcpy(item[i].Data, init[i].Data);
  }
}

int messageBox(u32 flags, char **items, int noItems, int noButtons)
{
  return (startupInfo.Message(startupInfo.ModuleNumber,
                              flags, NULL, items,
                              noItems, noButtons));
}

bool compareMemory(const void* p1, const void* p2, u16 size)
{
  return memcmp(p1, p2, size) == 0;
}

char* cropLongName(char* newName, const char* name, int noChars)
{
  int size = strlen(name);
  if(size > noChars)
  {
    if(name[1] == ':' && name[2] == '\\')
    {
      strncpy(newName, name, 4);
      strcat(newName, "...");
      strcat(newName, name+size-noChars+6);
    }
    else
    {
      strcpy(newName, "...");
      strcat(newName, name+size-noChars+3);
    }
  }
  else
    strcpy(newName, name);
  return newName;
}

bool isValidChar(char ch)
{
  if((unsigned char)ch >= 0x80 && (unsigned char)ch <= 0xAF) return true;
  if((unsigned char)ch >= 0xE0 && (unsigned char)ch <= 0xF1) return true;
  if((unsigned char)ch < 0x20 || (unsigned char)ch > 0x7F) return false;
  BYTE invalidChar[] = {' ', '.', '\\', '/', ':', '?', '*', '<', '>', '|'};
  for(int i = 0; i < sizeof(invalidChar); i++)
    if((unsigned char)ch == invalidChar[i]) return false;
  return true;
}

char* makeFullName(char* fullName, const char* path, const char* name)
{
  strcpy(fullName, path);
  addEndSlash(fullName);
  return strcat(fullName, name);
}

void makeCompatibleFileName(char* newName, const char* oldName)
{
  if(*oldName == 0) return;

  auto name866 = makeCp866Name(oldName);

  char* p = newName;
  const char* q = &name866[0];
  for(int i = 0; i < 8; ++i)
  {
    if(*q == 0 || *q == '.') break;
    if(isValidChar(*q))
      *p = *q;
    else
      *p = '_';
    
    ++p;
    ++q;
  }
  
  q = pointToExt(oldName);
  
  if(q)
  {
    *p++ = '.';
    for(int i = 0; i < 3; ++i)
    {
      if(*q == 0) break;
      if(isValidChar(*q))
        *p = *q;
      else
        *p = '_';
      ++p;
      ++q;
    }
  }
  *p = 0;
}

std::vector<char> makeCp866Name(const char* name)
{
  // name, which is obtained from functions, is actually UTF-8, convert it to multibyte
  auto nLength = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, NULL);
  std::vector<WCHAR> wstr1(nLength);
  MultiByteToWideChar(CP_UTF8, 0, name, -1, &wstr1[0], nLength);

  // and now convert multibyte to CP866
  std::vector<char> name866(nLength);
  WideCharToMultiByte(CP_OEMCP, 0, &wstr1[0], -1, &name866[0], nLength, NULL, NULL);
  return name866;
}

void makeCompatibleFolderName(char* newName, const char* oldName)
{
  if(*oldName == 0) return;

  auto oldName866 = makeCp866Name(oldName);

  char* p = newName;
  const char* q = &oldName866[0];
  for(int i = 0; i < 8; ++i)
  {
    if(*q == 0 || *q == '.') break;
    if(isValidChar(*q))
      *p = *q;
    else
      *p = '_';
    
    ++p;
    ++q;
  }
  *p = 0;
}
