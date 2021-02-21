#include <windows.h>
#include "trdos.hpp"
#include "tools.hpp"
#include "creator.hpp"
#include "lang.hpp"
#include "make.hpp"

extern PluginStartupInfo info;

HANDLE image;

char* pointToName(char *path)
{
  char* namePtr = path;
  while(*path)
  {
    if (*path == '\\' || *path == '/' || *path == ':') namePtr = path+1;
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

bool compareMemory(BYTE* p1, const BYTE* p2, WORD size)
{
  while(size--)
    if(*p1++ != *p2++) return false;
  return true;
}

enum FileType { FMT_SCL_, FMT_HOBETA, FMT_PLAIN };
BYTE signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R'};

FileType detectFileType(HANDLE file)
{
  DWORD fileSize = GetFileSize(file, NULL);
  
  DWORD noBytesRead;
  
  // проверяем уж не HoBeta ли это
  HoHdr hdr;
  ReadFile(file, &hdr, sizeof(HoHdr), &noBytesRead, 0);
  
  SetFilePointer(file, 0, NULL, FILE_BEGIN);
  if(hdr.checkSum == calculateCheckSum(hdr) &&
     fileSize == secSize*hdr.noSecs + sizeof(HoHdr)) return FMT_HOBETA;
  
  // проверяем уж не SCL ли это
  BYTE buf[sizeof(signature)];
  ReadFile(file, buf, sizeof(buf), &noBytesRead, 0);
  BYTE no_files;
  ReadFile(file, &no_files, 1, &noBytesRead, 0);
  
  SetFilePointer(file, 0, NULL, FILE_BEGIN);
  if(fileSize < no_files*(secSize+sizeof(SCLFileHdr))+8+1+4 ||
     !compareMemory(buf, signature, sizeof(signature))) return FMT_PLAIN;
  
  DWORD totalSecs = 0;
  SetFilePointer(file, 8+1, NULL, FILE_BEGIN);
  for(int i = 0; i < no_files; ++i)
  {
    SCLFileHdr fHdr;
    ReadFile(file, &fHdr, sizeof(fHdr), &noBytesRead, 0);
    totalSecs += fHdr.noSecs;
  }
  
  SetFilePointer(file, 0, NULL, FILE_BEGIN);
  if(fileSize < totalSecs*secSize+no_files*sizeof(SCLFileHdr)+8+1+4)
    return FMT_PLAIN;
  else
    return FMT_SCL_;
}

bool create(char *fileName, char *title, int writeProtect, int isDS, int format, int interleave, char *bootName, char *comment)
{
  BYTE   DSsignature[] = {'D', 'i', 'r', 'S', 'y', 's', '1', '0', '0'};
  int    noFiles   = 0;
  int    totalSecs = 0;
  HANDLE boot      = INVALID_HANDLE_VALUE;
  int    i;

  DWORD noBytesRead;
  
  Track0 track0;
  ZeroMemory(&track0, sizeof(Track0));

  HANDLE screen = info.SaveScreen(0,0,-1,-1);

  if(bootName)
  {
    boot = CreateFile(bootName,
                      GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);
    if(boot == INVALID_HANDLE_VALUE)
    {
      char *msgItems[4];
      msgItems[0] = getMsg(MError);
      msgItems[1] = getMsg(MCanNotOpen);
      msgItems[2] = bootName;
      msgItems[3] = getMsg(MOk);
      info.Message(info.ModuleNumber,
                   FMSG_WARNING, NULL,
                   msgItems,
                   sizeof(msgItems)/sizeof(msgItems[0]), 1);
    }
    else
    {
      FileType t = detectFileType(boot);
      if(t == FMT_PLAIN)
      {
        char *msgItems[4];
        msgItems[0] = getMsg(MError);
        msgItems[1] = bootName;
        msgItems[2] = getMsg(MUnknown);
        msgItems[3] = getMsg(MOk);
        info.Message(info.ModuleNumber,
                     FMSG_WARNING, NULL,
                     msgItems,
                   sizeof(msgItems)/sizeof(msgItems[0]), 1);
      }
      else
      {
        if(t == FMT_HOBETA)
        {
          HoHdr hdr;
          ReadFile(boot, &hdr, sizeof(HoHdr), &noBytesRead, 0);
          CopyMemory(&track0.files[0], &hdr, sizeof(HoHdr)-4);
          totalSecs = hdr.noSecs;
          track0.files[0].noSecs = hdr.noSecs;
          noFiles = 1;
        }
        else
        {
          SetFilePointer(boot, 8, NULL, FILE_BEGIN);
          BYTE no_files;
          ReadFile(boot, &no_files, 1, &noBytesRead, 0);
          for(i = 0; i < no_files; ++i)
          {
            SCLFileHdr hdr;
            ReadFile(boot, &hdr, sizeof(SCLFileHdr), &noBytesRead, 0);
            CopyMemory(&track0.files[i], &hdr, sizeof(SCLFileHdr));
            totalSecs += hdr.noSecs;
          }
          noFiles = no_files;
          if((format != FMT_SCL) && ((noFiles > 128) || totalSecs > 2544))
          {
            char *msgItems[4];
            msgItems[0] = getMsg(MError);
            msgItems[1] = bootName;
            msgItems[2] = getMsg(MTooBig);
            msgItems[3] = getMsg(MOk);
            info.Message(info.ModuleNumber,
                         FMSG_WARNING, NULL,
                         msgItems,
                         sizeof(msgItems)/sizeof(msgItems[0]), 1);
            noFiles = 0;
          }
        }
      }
    }
  }

  char* ext = 0;
  switch(format)
  {
    case FMT_TRD:
              ext = ".trd";
              break;
    case FMT_FDI:
              ext = ".fdi";
              break;
    case FMT_SCL:
              ext = ".scl";
              break;
    case FMT_FDD:
              ext = ".fdd";
              break;
    case FMT_UDI:
              ext = ".udi";
              break;
    case FMT_TD:
              ext = ".td0";
              break;
  }
  if(!pointToExt(fileName)) lstrcat(fileName, ext);

  image = CreateFile(fileName,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_NEW,
                     FILE_FLAG_SEQUENTIAL_SCAN,
                     NULL);
  if(image == INVALID_HANDLE_VALUE)
  {
    char *msgItems[4];
    msgItems[0] = getMsg(MError);
    msgItems[1] = getMsg(MCanNotCreate);
    msgItems[2] = fileName;
    msgItems[3] = getMsg(MOk);
    info.Message(info.ModuleNumber,
                 FMSG_WARNING, NULL,
                 msgItems,
                 sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return false;
  }

  if((format != FMT_FDI) && writeProtect)
    SetFileAttributes(fileName, FILE_ATTRIBUTE_READONLY);

  char* msgItems[2];
  msgItems[0] = NULL;
  msgItems[1] = getMsg(MCreating);
  info.Message(info.ModuleNumber, 0, NULL, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);

  int trk = 1;
  int sec = 0;
  int noDelFiles = 0;
  for(i = 0; i < noFiles; ++i)
  {
    track0.files[i].trk = trk;
    track0.files[i].sec = sec;
    if(track0.files[i].name[0] == 0x01) ++noDelFiles;
    trk += track0.files[i].noSecs/noSecs;
    sec += track0.files[i].noSecs%noSecs;
    if(sec > noSecs-1) { ++trk; sec -= noSecs; }
  }
  track0.flag         = 0x10;
  track0.type         = 0x16;
  track0.firstFreeTrk = trk;
  track0.firstFreeSec = sec;
  track0.noFiles      = noFiles;
  track0.noDelFiles   = noDelFiles;
  track0.noFreeSecs   = 2544 - totalSecs;

  FillMemory(track0.title, 11, ' ');
  for(i = 0; i < 11; ++i)
  {
    if(title[i] == 0) break;
    if(title[i] > 0) track0.title[i] = title[i];
  }

  if(isDS)
  {
    track0.DScrc = 0xD019;
    for(int i = 0; i < 9; ++i)
      track0.DSsignature[i] = DSsignature[i];
  }

  BYTE sectors[2][16] ={{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                        {1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15, 8, 16}};

  switch(format)
  {
    case FMT_TRD:
              createTRD(track0, totalSecs, image, boot);
              break;
    case FMT_FDI:
              createFDI(track0, totalSecs, sectors[interleave-1], writeProtect, image, boot, comment);
              break;
    case FMT_SCL:
              createSCL(track0, totalSecs, image, boot);
              break;
    case FMT_FDD:
              createFDD(track0, totalSecs, sectors[interleave-1], image, boot);
              break;
    case FMT_UDI:
              createUDI(track0, totalSecs, sectors[interleave-1], image, boot, comment);
              break;
    case FMT_TD:
              createTD(track0, totalSecs, sectors[interleave-1], image, boot, comment);
              break;
  }

  if(boot != INVALID_HANDLE_VALUE) CloseHandle(boot);
  CloseHandle(image);
  info.RestoreScreen(screen);
  return true;
}
