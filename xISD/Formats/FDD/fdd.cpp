#include "..\fmt.hpp"
#include "filer.hpp"
#include "fdd.hpp"

bool WINAPI _export isImage(char* fileName, const BYTE* data, int size)
{
  BYTE *ID = "SPM DISK (c) 1996 MOA v0.1    ";
  for (BYTE i = 0; i< versionLength; i++)
    if (data[i] != ID[i])
      return false;

  HANDLE hostFile = CreateFile(fileName,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return false;

  DWORD noBytesRead;
  FDDHdr hFDD;
  ReadFile(hostFile, &hFDD, sizeof(FDDHdr), &noBytesRead, 0);
  SetFilePointer(hostFile, hFDD.cylsOffset[0], NULL, FILE_BEGIN);

  TrackHdr hTrk;
  ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);

  int sec;
  SectorHdr hSec;
  for(sec = 0; sec < hTrk.noSecs; ++sec)
  {
    ReadFile(hostFile, &hSec, sizeof(SectorHdr), &noBytesRead, 0);
    if((hSec.r == 1) && (hSec.n > 0)) break;
  }

  if (sec == hTrk.noSecs)
  {
    CloseHandle(hostFile);
    return false;
  }

  SetFilePointer(hostFile, hSec.offset, NULL, FILE_BEGIN);

  BYTE buf[blockSize];
  ReadFile(hostFile, buf, blockSize, &noBytesRead, 0);
  CloseHandle(hostFile);
  
  if((buf[10] != 'D' || buf[11] != 'S' || buf[12] != 'K') &&
     (buf[13] != 'D' || buf[14] != 'S' || buf[15] != 'K')) return false;
  return true;
}

HANDLE WINAPI _export openSubPlugin(char* fileName)
{
  return (HANDLE)(new Filer(fileName));
}

void WINAPI _export closeSubPlugin(HANDLE h)
{
  delete (Filer*)h;
}

bool WINAPI _export reload (HANDLE h) { return true; }

bool WINAPI _export openFile(HANDLE h)
{
  Filer* f = (Filer*)h;
  return f->openFile();
}

bool WINAPI _export closeFile(HANDLE h)
{
  Filer* f = (Filer*)h;
  return f->closeFile();
}

bool WINAPI _export read(HANDLE h, WORD blockNum, BYTE* buf)
{
  Filer* f = (Filer*)h;
  return f->read(blockNum, buf);
}

bool WINAPI _export write(HANDLE h, WORD blockNum, BYTE* buf)
{
  Filer* f = (Filer*)h;
  return f->write(blockNum, buf);
}

char* WINAPI _export getFormatName (void)
{
  static char* name = "FDD";
  return name;
}
