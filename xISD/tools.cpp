#include <windows.h>
#include "plugin.hpp"
#include "tools.hpp"

extern PluginStartupInfo startupInfo;

bool compareMemoryIgnoreCase(u8* p1, u8* p2, u16 size)
{
  u8 ch1, ch2;
  while(size--)
  {
    ch1 = (u8)CharUpper((char*)*p1++);
    ch2 = (u8)CharUpper((char*)*p2++);
    if(ch1 != ch2) return false;
  }
  return true;
}

char* pointToName(char* path)
{
  char *namePtr = path;
  while(*path)
  {
    if(*path == '\\' || *path == '/' || *path==':') namePtr = path+1;
    path++;
  }
  return namePtr;
}

char* pointToExt(char* path)
{
  int i = lstrlen(path);
  char *ptr = path + i;
  while(*ptr != '.' && i != 0) { ptr--; i--; }
  if(i)
    return (ptr+1);
  else
    return 0;
}


void addEndSlash(char *path)
{
  if(!path) return;
  int length = lstrlen(path);
  if(length != 0 && path[length-1] == ':') return;
  if(length != 0 && path[length-1] != '\\') lstrcat(path,"\\");
}

char* getMsg(int msgId)
{
  return(startupInfo.GetMsg(startupInfo.ModuleNumber, msgId));
}

void initDialogItems(InitDialogItem *init, FarDialogItem *item, int noItems)
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
    if((unsigned int)init[i].Data < 2000)
      lstrcpy(item[i].Data, getMsg((unsigned int)init[i].Data));
    else
      lstrcpy(item[i].Data, init[i].Data);
  }
}

int messageBox(u32 flags, char **items, int noItems, int noButtons)
{
  return (startupInfo.Message(startupInfo.ModuleNumber,
                              flags, NULL, items,
                              noItems, noButtons));
}

bool compareMemory(void* p1, void* p2, u16 size)
{
  while(size--)
    if(*((u8*)p1)++ != *((u8*)p2)++) return false;
  return true;
}

char* cropLongName(char* newName, const char* name, int noChars)
{
  int size = lstrlen(name);
  if(size > noChars)
  {
    if(name[1] == ':' && name[2] == '\\')
    {
      lstrcpyn(newName, name, 4);
      lstrcat(newName, "...");
      lstrcat(newName, name+size-noChars+6);
    }
    else
    {
      lstrcpy(newName, "...");
      lstrcat(newName, name+size-noChars+3);
    }
  }
  else
    lstrcpy(newName, name);
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
  lstrcpy(fullName, path);
  addEndSlash(fullName);
  return lstrcat(fullName, name);
}

void makeCompatibleFileName(char* newName, char* oldName)
{
  if(*oldName == 0) return;
  char* p = newName;
  char* q = oldName;
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

void makeCompatibleFolderName(char* newName, char* oldName)
{
  if(*oldName == 0) return;
  char* p = newName;
  char* q = oldName;
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
