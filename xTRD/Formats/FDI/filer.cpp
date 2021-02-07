#include <windows.h>
#include "filer.hpp"
#include "fdi.hpp"

Filer::Filer(const char* fileName)
{
  lstrcpy(fName, fileName);
  open();

  DWORD noBytesRead;
  FDIHdr hFDI;
  ReadFile(hostFile, &hFDI, sizeof(FDIHdr), &noBytesRead, 0);
  
  writeProtection = hFDI.writeProtection;
  maxSec = 16*hFDI.noCyls*hFDI.noHeads;
  secs = new DWORD[maxSec];
  ZeroMemory(secs, maxSec);
  
  SetFilePointer(hostFile, sizeof(FDIHdr)+hFDI.extraInfoSize, NULL, FILE_BEGIN);
  
  for(int trk = 0; trk < hFDI.noCyls*hFDI.noHeads; ++trk)
  {
    TrackHdr hTrk;
    ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);
    for(BYTE sec = 0; sec < hTrk.noSecs; ++sec)
    {
      SectorHdr hSec;
      ReadFile(hostFile, &hSec, sizeof(SectorHdr), &noBytesRead, 0);
      if((hSec.n != 1) || (hSec.r < 1) || (hSec.r > 16)) continue;

      secs[16*trk+hSec.r-1] = hFDI.dataOffset + hTrk.offset + hSec.offset;
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
  return (writeProtection || attr & FILE_ATTRIBUTE_READONLY);
}

bool Filer::protect(bool on)
{
  if(!on) 
    if(!SetFileAttributes(fName, FILE_ATTRIBUTE_NORMAL))
      return false;

  if(!open()) return false;
  writeProtection = on ? 1 : 0;
  SetFilePointer(hostFile, 3, NULL, FILE_BEGIN);
  DWORD noBytesWritten;
  WriteFile(hostFile, &writeProtection, 1, &noBytesWritten, NULL);
  close();
  return (noBytesWritten == 1);
}
