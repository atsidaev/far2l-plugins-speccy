#include "../fmt.hpp"
#include "filer.hpp"
#include "fdd.hpp"
#include "../../../shared/widestring.hpp"

bool WINAPI _export fdd_isImage(const char* fileName, const BYTE* data, int size)
{
  BYTE *ID = (BYTE*)"SPM DISK (c) 1996 MOA v0.1    ";
  for (BYTE i = 0; i< versionLength; i++)
    if (data[i] != ID[i])
      return false;

  HANDLE hostFile = CreateFile(_W((char*)fileName).c_str(),
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

  SectorHdr *hSecs = new SectorHdr[hTrk.noSecs];
  WORD mask = 0;
  WORD sysOffset = 0;
  
  ReadFile(hostFile, hSecs, sizeof(SectorHdr)*hTrk.noSecs, &noBytesRead, 0);
  for(BYTE sec = 0; sec < hTrk.noSecs; ++sec)
  {
    if(hSecs[sec].n == 1 &&
       hSecs[sec].r >  0 &&
       hSecs[sec].r <  17) mask |= (1<<(hSecs[sec].r-1));
    if(hSecs[sec].n == 1 &&
       hSecs[sec].r == 9) sysOffset = hSecs[sec].offset;
  }

  delete[] hSecs;

  if(mask != 0xFFFF)
  { 
    CloseHandle(hostFile);
    return false;
  }

  BYTE signature = 0;
  SetFilePointer(hostFile, sysOffset + 0xE7, NULL, FILE_BEGIN);

  ReadFile(hostFile, &signature, 1, &noBytesRead, NULL);
  CloseHandle(hostFile);
  
  if(signature != 0x10) return false;

  return true;
}

HANDLE WINAPI _export fdd_init(const char* fileName)
{
  return (HANDLE)(new FilerFDD(fileName));
}

void WINAPI _export fdd_cleanup(HANDLE h)
{
  delete (FilerFDD*)h;
}

bool WINAPI _export fdd_reload (HANDLE h) { return true; }

bool WINAPI _export fdd_open(HANDLE h)
{
  FilerFDD* f = (FilerFDD*)h;
  return f->open();
}

bool WINAPI _export fdd_close(HANDLE h)
{
  FilerFDD* f = (FilerFDD*)h;
  return f->close();
}

bool WINAPI _export fdd_read(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  FilerFDD* f = (FilerFDD*)h;
  return f->read(trk, sec, buf);
}

bool WINAPI _export fdd_write(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  FilerFDD* f = (FilerFDD*)h;
  return f->write(trk, sec, buf);
}

char* WINAPI _export fdd_getFormatName (void)
{
  static char* name = "FDD";
  return name;
}

bool WINAPI _export fdd_isProtected(HANDLE h)
{
  FilerFDD* f = (FilerFDD*)h;
  return f->isProtected();
}

bool WINAPI _export fdd_protect(HANDLE h, bool on)
{
  FilerFDD* f = (FilerFDD*)h;
  return f->protect(on);
}
