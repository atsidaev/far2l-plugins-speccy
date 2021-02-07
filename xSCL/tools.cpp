#include <windows.h>
#include "plugin.hpp"

#include "tools.hpp"
#include "types.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

bool compareMemory(BYTE* p1, BYTE* p2, WORD size)
{
  while(size--)
    if(*p1++ != *p2++) return false;
  return true;
}

bool compareMemoryIgnoreCase(BYTE* p1, BYTE* p2, WORD size)
{
  BYTE ch1, ch2;
  while(size--)
  {
    ch1 = (BYTE)CharUpper((char*)*p1++);
    ch2 = (BYTE)CharUpper((char*)*p2++);
    if(ch1 != ch2) return false;
  }
  return true;
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

char* pointToExt(char *path)
{
  int i = lstrlen(path);
  char *ptr = path + i;
  while(*ptr != '.' && i != 0) { ptr--; i--; }
  if(i)
    return (ptr+1);
  else
    return 0;
}

bool isValidChar(BYTE ch)
{
  if(ch < 0x20 || ch > 0x7F) return false;
  BYTE invalidChar[] = {' ', '.', '\\', '/', ':', '?', '*', '<', '>', '|'};
  for(int i = 0; i < sizeof(invalidChar); i++)
    if(ch == invalidChar[i]) return false;
  return true;
}

bool isAlphaNumeric(BYTE ch)
{
  if((ch>='0' && ch <='9') ||
     (ch>='a' && ch <='z') ||
     (ch>='A' && ch <='Z'))
    return true;
  else
    return false;
}

void addEndSlash(char *path)
{
  int length=lstrlen(path);
  if(length == 0 || path[length-1] != '\\') lstrcat(path,"\\");
}

char* trim(char *str)
{
  int i,j,len;

  len = lstrlen(str);

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

DWORD copySectors(HANDLE to, HANDLE from, int noSecs)
{
  DWORD noBytesWritten, noBytesRead;
  DWORD checkSum = 0;
  BYTE  sector[sectorSize];
  while(noSecs--)
  {
    ZeroMemory(sector, sectorSize);
    ReadFile (from, sector, sectorSize, &noBytesRead, NULL);
    WriteFile(to,   sector, sectorSize, &noBytesWritten, NULL);
    checkSum += calculateCheckSum(sector, sectorSize);
  }
  return checkSum;
}

bool isSCLFull(int no_files, int no_add_files, HANDLE file)
{
  if(no_files + no_add_files > 255)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MTooBig1);
    msgItems[2] = getMsg(MTooBig2);
    msgItems[3] = getMsg(MOk);
    
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    CloseHandle(file);
    return true;
  }
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
     isAlphaNumeric(ch1) &&
     isAlphaNumeric(ch2) &&
     isAlphaNumeric(ch3))
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

void errorMessage(char *msg)
{
  char *msgItems[3];
  msgItems[0] = getMsg(MError);
  msgItems[1] = msg;
  msgItems[2] = getMsg(MOk);
  startupInfo.Message(startupInfo.ModuleNumber,
                      FMSG_WARNING, NULL,
                      msgItems,
                      sizeof(msgItems)/sizeof(msgItems[0]), 1);
}

int messageBox(DWORD flags, char **items, int noItems, int noButtons)
{
  return (startupInfo.Message(startupInfo.ModuleNumber,
                              flags, NULL, items,
                              noItems, noButtons));
}

FileType detectFileType(HANDLE file)
{
  BYTE  signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R' };
  DWORD fileSize = GetFileSize(file, NULL);
  
  DWORD noBytesRead;
  
  // проверяем уж не HoBeta ли это
  HoHdr hdr;
  ReadFile(file, &hdr, sizeof(HoHdr), &noBytesRead, 0);

  SetFilePointer(file, 0, NULL, FILE_BEGIN);
  if(hdr.checkSum == calculateCheckSum(hdr) &&
     fileSize >= sectorSize*hdr.noSecs + sizeof(HoHdr)) return FMT_HOBETA;
  
  // проверяем уж не SCL ли это
  BYTE buf[sizeof(signature)];
  ReadFile(file, buf, sizeof(buf), &noBytesRead, 0);
  BYTE no_files;
  ReadFile(file, &no_files, 1, &noBytesRead, 0);
  
  SetFilePointer(file, 0, NULL, FILE_BEGIN);
  if(fileSize < no_files*(sectorSize+sizeof(FileHdr))+8+1+4 ||
     !compareMemory(buf, signature, sizeof(signature))) return FMT_PLAIN;
  
  DWORD totalSecs = 0;
  SetFilePointer(file, 8+1, NULL, FILE_BEGIN);
  for(int i = 0; i < no_files; ++i)
  {
    FileHdr fHdr;
    ReadFile(file, &fHdr, sizeof(fHdr), &noBytesRead, 0);
    totalSecs += fHdr.noSecs;
  }
  
  SetFilePointer(file, 0, NULL, FILE_BEGIN);
  if(fileSize < totalSecs*sectorSize+no_files*sizeof(FileHdr)+8+1+4)
    return FMT_PLAIN;
  else
    return FMT_SCL;
}
