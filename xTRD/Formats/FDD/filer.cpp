#include <windows.h>
#include "filer.hpp"
#include "fdd.hpp"
#include "../../../shared/widestring.hpp"

FilerFDD::FilerFDD(const char* fileName)
{
  strcpy(fName, fileName);
  open();

  DWORD noBytesRead;
  FDDHdr hFDD;
  ReadFile(hostFile, &hFDD, sizeof(FDDHdr), &noBytesRead, 0);
  
  maxSec = 16*hFDD.noCyls*hFDD.noHeads;
  secs = new DWORD[maxSec];
  memset(secs, 0, maxSec);
  
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

FilerFDD::~FilerFDD()
{
  delete[] secs;
}

bool FilerFDD::open(void)
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

bool FilerFDD::close(void)
{
  return CloseHandle(hostFile);
}

bool FilerFDD::read(BYTE trk, BYTE sec, BYTE* buf)
{
  memset(buf, 0, sectorSize);
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  SetFilePointer(hostFile, secs[16*trk+sec], NULL, FILE_BEGIN);
  DWORD noBytesRead;
  ReadFile(hostFile, buf, sectorSize, &noBytesRead, NULL);
  return (noBytesRead == sectorSize);
}

bool FilerFDD::write(BYTE trk, BYTE sec, BYTE* buf)
{
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  SetFilePointer(hostFile, secs[16*trk+sec], NULL, FILE_BEGIN);
  DWORD noBytesWritten;
  WriteFile(hostFile, buf, sectorSize, &noBytesWritten, NULL);
  return (noBytesWritten == sectorSize);
}

bool FilerFDD::isProtected(void)
{
  DWORD attr = GetFileAttributes(_W(fName).c_str());
  return (attr & FILE_ATTRIBUTE_READONLY);
}

bool FilerFDD::protect(bool on)
{
  DWORD attr = GetFileAttributes(_W(fName).c_str());
  if(on)
    attr |= FILE_ATTRIBUTE_READONLY;
  else
    attr &= ~FILE_ATTRIBUTE_READONLY;

  return (SetFileAttributes(_W(fName).c_str(), attr));
}
