#include <windows.h>
#include "pluginold.hpp"
using namespace oldfar;
#include "tools.hpp"
#include "types.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

void errorMessage(char *msg)
{
  char *msgItems[]={"Error", "", "Ok"};
  msgItems[1] = msg;
  startupInfo.Message(startupInfo.ModuleNumber,
                      FMSG_WARNING, NULL,
                      msgItems,
                      sizeof(msgItems)/sizeof(msgItems[0]), 1);
}

int messageBox(unsigned long flags, char **items, int noItems, int noButtons)
{
  return (startupInfo.Message(startupInfo.ModuleNumber,
                              flags, NULL, items,
                              noItems, noButtons));
}

char* pointToName(char *path)
{
  char *namePtr = path;
  while(*path)
  {
    if(*path == '\\' || *path == '/' || *path==':') namePtr = path+1;
    path++;
  }
  return namePtr;
}

char* trim(char *str)
{
  int i,j,len;

  len = strlen(str);

  for(i = len-1; i>=0; i--)
    if(str[i] != ' ') break;
  str[i+1] = 0;

  for(i = 0; i < len; i++)
    if(str[i] != ' ') break;
  if(i == 0) return str;

  for(j = i; j < len; j++)
    str[j-i] = str[j];

  return str;
}

char* truncPathStr(char* newName, const char* name, int noChars)
{
  int size = strlen(name);
  if(size > noChars)
  {
    if(name[1] == ':' && name[2] == '/')
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

void addEndSlash(char *path)
{
  int length=strlen(path);
  if(length == 0 || path[length-1] != '/') strcat(path,"/");
}

void quoteSpaceOnly(char *str)
{
  char *tmp = str;
  char to[300];
  int len = strlen(str);

  memcpy(to, str, len);
  *(to + len) = 0;

  while(*tmp)
  {
    if(*tmp == ' ')
    {
      *to = '"';
      memcpy(to + 1, str, len);
      *(to + len + 1) = '"';
      *(to + len + 2) = 0;
      break;
    }
    ++tmp;
  }

  strcpy(str, to);
}

bool isValidChar(BYTE ch)
{
  if(ch < 0x20 || ch > 0x7F) return false;
  BYTE invalidChar[] = {' ', '.', '\\', '/', ':', '?', '*', '<', '>', '|'};
  for(int i = 0; i < sizeof(invalidChar); i++)
    if(ch == invalidChar[i]) return false;
  return true;
}

bool isValidFolderChar(BYTE ch)
{
  if(ch >= 0x80 && ch <= 0xAF) return true;
  if(ch >= 0xE0 && ch <= 0xF1) return true;
  if(ch < 0x20 || ch > 0x7F) return false;
  BYTE invalidChar[] = {' ', '.', '\\', '/', ':', '?', '*', '<', '>', '|'};
  for(int i = 0; i < sizeof(invalidChar); i++)
    if(ch == invalidChar[i]) return false;
  return true;
}

/*int memcmp(const BYTE* p1, const BYTE* p2, int maxlen)
{
  if(!maxlen) return(0);
  while(maxlen-- && *p1 == *p2) { p1++; p2++; }
  return(*p1 - *p2);
}*/

int strcmpi(const char* p1, const char* p2)
{
  char ch1, ch2;
  
  while (true)
  {
    ch1 = (char)(long)((char*)*p1++);
    ch2 = (char)(long)((char*)*p2++);
    
    if ((ch1 == 0 || ch2 == 0) && ch1 != ch2)
      break;
    
    if (ch1 >= 'a' && ch1 <= 'z')
      ch1 -= 'a' - 'A';
    
    if (ch2 >= 'a' && ch2 <= 'z')
      ch2 -= 'a' - 'A';
    
    if(ch1 != ch2) break;
  }
  return (ch1 - ch2);
}

int memcmpi(const char* p1, const char* p2, int maxlen)
{
  if(!maxlen) return(0);
  char ch1, ch2;
  
  while(maxlen--)
  {
    ch1 = (char)(long)((char*)*p1++);
    ch2 = (char)(long)((char*)*p2++);
    
    if (ch1 >= 'a' && ch1 <= 'z')
      ch1 -= 'a' - 'A';
    
    if (ch2 >= 'a' && ch2 <= 'z')
      ch2 -= 'a' - 'A';
    
    if(ch1 != ch2) break;
  }
  return (ch1 - ch2);
}

char *str_r_chr(const char *s, int c)
{
  const char *ss;
  int         i;

  for(i = strlen( s ) + 1, ss = s+i; i; i--)
    if( *(--ss) == (char)c )  return( (char *)ss );
  return( 0 );
}

bool isEnAlphaNum(BYTE ch)
{
  if(ch >= '0' && ch <= '9') return true;
  if(ch >= 'A' && ch <= 'Z') return true;
  if(ch >= 'a' && ch <= 'z') return true;
  return false;
}

void makeTrDosName(char* dest, const FileHdr& hdr, int width)
{
  for(int i = 0; i < 8; ++i) dest[i] = hdr.name[i] ? hdr.name[i] : ' ';
  
  BYTE ch1, ch2, ch3;
  ch1 = hdr.type;
  ch2 = hdr.start & 0xff;
  ch3 = hdr.start>>8;
  
  if(op.showExt &&
     isEnAlphaNum(ch1) &&
     isEnAlphaNum(ch2) &&
     isEnAlphaNum(ch3))
  {
    dest[width-3] = ch1;
    dest[width-2] = ch2;
    dest[width-1] = ch3;
  }
  else
  {
    dest[width-3] = '<';
    dest[width-2] = ch1 ? ch1 : ' ';
    dest[width-1] = '>';
  }
  dest[width] = 0;
}

WORD crc(BYTE* ptr, WORD len)
{
  WORD crc = 0;
  
  for(; len; len--)
  {
    BYTE tmp = crc ^ *ptr++;
    
    WORD bCRC = 0;
    for(int i = 0; i < 8; ++i)
    {
      WORD lBit = bCRC & 1;
      bCRC >>= 1; bCRC |= lBit<<15; // RRA crc

      if((tmp & 1) ^ lBit) bCRC ^= 0xA001;
      tmp >>= 1;
    }
    crc = bCRC ^ (crc<<8 | crc>>8);
  }
  return (crc<<8 | crc>>8);
}

DWORD calculateCheckSum(BYTE* ptr, WORD size)
{
  DWORD sum = 0;
  while(size--) sum += *ptr++;
  return sum;
}

WORD calculateCheckSum(HoHdr& hdr)
{
  WORD sum = 0;
  BYTE* p = (BYTE*)&hdr;
  for(int i = 0; i < sizeof(HoHdr)-2; ++i) sum += *p++;
  return 257*sum + 105;
}

char* make8x3name(const char* source, char* dest)
{
  const char* ptr = source;
  // копируем имя
  for(int i = 0; i < 8; ++i)
  {
    if(!*ptr || *ptr == '.') break;
    dest[i] = *ptr++;
  }
  // если имя слишком длинное - пропускаем остаток
  while(*ptr && *ptr != '.') ++ptr;

  if(*ptr == '.')
  {
    ++ptr;
    // копируем расширение
    for(int i = 0; i < 3; ++i)
    {
      if(!*ptr) break;
      dest[8+i] = *ptr++;
    }
  }
  return dest;
}

char* getMsg(int msgId)
{
  return((char*)startupInfo.GetMsg(startupInfo.ModuleNumber, msgId));
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
    item[i].Reserved      = init[i].Selected;
    item[i].Flags         = init[i].Flags;
    item[i].DefaultButton = init[i].DefaultButton;
    if((unsigned long)init[i].Data < 2000)
      strcpy(item[i].Data, getMsg((unsigned long)init[i].Data));
    else
      strcpy(item[i].Data, init[i].Data);
  }
}
