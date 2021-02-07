#include <windows.h>
#include "filer.hpp"
#include "fdi.hpp"
#include "../../../shared/widestring.hpp"

FilerFDI::FilerFDI(const char* fileName)
{
  strcpy(fName, fileName);
  open();

  DWORD noBytesRead;
  FDIHdr hFDI;
  ReadFile(hostFile, &hFDI, sizeof(FDIHdr), &noBytesRead, 0);
  
  writeProtection = hFDI.writeProtection;
  maxSec = 16*hFDI.noCyls*hFDI.noHeads;
  secs = new DWORD[maxSec];
  memset(secs, 0, maxSec);
  
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

FilerFDI::~FilerFDI()
{
  delete[] secs;
}

bool FilerFDI::open(void)
{
  DWORD mode = GENERIC_READ | GENERIC_WRITE;
  DWORD attr = GetFileAttributes(_W(fName).c_str());
  if(attr & FILE_ATTRIBUTE_READONLY) mode = GENERIC_READ;
  hostFile = CreateFile(_W(fName).c_str(),
                        mode,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);
  return (hostFile != INVALID_HANDLE_VALUE);
}

bool FilerFDI::close(void)
{
  return CloseHandle(hostFile);
}

bool FilerFDI::read(BYTE trk, BYTE sec, BYTE* buf)
{
  memset(buf, 0, sectorSize);
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  SetFilePointer(hostFile, secs[16*trk+sec], NULL, FILE_BEGIN);
  DWORD noBytesRead;
  ReadFile(hostFile, buf, sectorSize, &noBytesRead, NULL);
  return (noBytesRead == sectorSize);
}

bool FilerFDI::write(BYTE trk, BYTE sec, BYTE* buf)
{
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  SetFilePointer(hostFile, secs[16*trk+sec], NULL, FILE_BEGIN);
  DWORD noBytesWritten;
  WriteFile(hostFile, buf, sectorSize, &noBytesWritten, NULL);
  return (noBytesWritten == sectorSize);
}

bool FilerFDI::isProtected(void)
{
  DWORD attr = GetFileAttributes(_W(fName).c_str());
  return (writeProtection || attr & FILE_ATTRIBUTE_READONLY);
}

bool FilerFDI::protect(bool on)
{
  if(!on) 
    if(!SetFileAttributes(_W(fName).c_str(), FILE_ATTRIBUTE_NORMAL))
      return false;

  if(!open()) return false;
  writeProtection = on ? 1 : 0;
  SetFilePointer(hostFile, 3, NULL, FILE_BEGIN);
  DWORD noBytesWritten;
  WriteFile(hostFile, &writeProtection, 1, &noBytesWritten, NULL);
  close();
  return (noBytesWritten == 1);
}
