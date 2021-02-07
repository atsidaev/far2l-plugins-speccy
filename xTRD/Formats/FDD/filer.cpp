#include <windows.h>
#include "filer.hpp"
#include "fdd.hpp"

Filer::Filer(const char* fileName)
{
  lstrcpy(fName, fileName);
  open();

  DWORD noBytesRead;
  FDDHdr hFDD;
  ReadFile(hostFile, &hFDD, sizeof(FDDHdr), &noBytesRead, 0);
  
  maxSec = 16*hFDD.noCyls*hFDD.noHeads;
  secs = new DWORD[maxSec];
  ZeroMemory(secs, maxSec);
  
  for(int trk = 0; trk < hFDD.noCyls*hFDD.noHeads; ++trk)
  {
    TrackHdr hTrk;
    SetFilePointer(hostFile, hFDD.cylsOffset[trk], NULL, FILE_BEGIN);
    ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);
    for(BYTE sec = 0; sec < hTrk.noSecs; ++sec)
    {
      SectorHdr hSec;
      ReadFile(hostFile, &hSec, sizeof(SectorHdr), &noBytesRead, 0);
      if((hSec.n != 1) || (hSec.r < 1) || (hSec.r > 16)) continue;

      secs[16*trk+hSec.r-1] = hSec.offset;
    }
  }
  close();
}

Filer::~Filer()
{
  delete[] secs;
}

bool Filer::open(void)
{
  DWORD mode = GENERIC_READ | GENERIC_WRITE;
  DWORD attr = GetFileAttributes(fName);
  if(attr & FILE_ATTRIBUTE_READONLY) mode = GENERIC_READ;
  hostFile = CreateFile(fName,
                        mode,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);
  return (hostFile != INVALID_HANDLE_VALUE);
}

bool Filer::close(void)
{
  return CloseHandle(hostFile);
}

bool Filer::read(BYTE trk, BYTE sec, BYTE* buf)
{
  ZeroMemory(buf, sectorSize);
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  SetFilePointer(hostFile, secs[16*trk+sec], NULL, FILE_BEGIN);
  DWORD noBytesRead;
  ReadFile(hostFile, buf, sectorSize, &noBytesRead, NULL);
  return (noBytesRead == sectorSize);
}

bool Filer::write(BYTE trk, BYTE sec, BYTE* buf)
{
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  SetFilePointer(hostFile, secs[16*trk+sec], NULL, FILE_BEGIN);
  DWORD noBytesWritten;
  WriteFile(hostFile, buf, sectorSize, &noBytesWritten, NULL);
  return (noBytesWritten == sectorSize);
}

bool Filer::isProtected(void)
{
  DWORD attr = GetFileAttributes(fName);
  return (attr & FILE_ATTRIBUTE_READONLY);
}

bool Filer::protect(bool on)
{
  DWORD attr = GetFileAttributes(fName);
  if(on)
    attr |= FILE_ATTRIBUTE_READONLY;
  else
    attr &= ~FILE_ATTRIBUTE_READONLY;

  return (SetFileAttributes(fName, attr));
}
