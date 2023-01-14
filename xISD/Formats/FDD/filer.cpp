#include <windows.h>
#include "filer.hpp"
#include "fdd.hpp"

Filer::Filer(char* fileName)
{
  lstrcpy(fName, fileName);
  openFile();

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
    if(hSec.r == 1) break;
  }

  SetFilePointer(hostFile, hSec.offset, NULL, FILE_BEGIN);

  BYTE buf[blockSize];
  ReadFile(hostFile, buf, blockSize, &noBytesRead, 0);

  BYTE sectorSize = buf[0x18];
  BYTE sectorType = 255;
  switch(sectorSize)
  {
    case 1:
              sectorType = 1;
              break;
    case 2:
              sectorType = 2;
              break;
    case 4:
              sectorType = 3;
              break;
  }
  
  BYTE noSecs   = buf[0x19];
  WORD noBlocks = *(WORD*)(buf+0x12);
  blks = (DWORD*)malloc(4*noBlocks);
  ZeroMemory(blks, 4*noBlocks);
  
  BYTE noBlksOnTrk = noSecs*sectorSize;

  for(int trk = 0; trk < hFDD.noCyls*hFDD.noHeads; ++trk)
  {
    SetFilePointer(hostFile, hFDD.cylsOffset[trk], NULL, FILE_BEGIN);
    ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);
    for(int sec = 0; sec < hTrk.noSecs; ++sec)
    {
      ReadFile(hostFile, &hSec, sizeof(SectorHdr), &noBytesRead, 0);
      if(hSec.n != sectorType) continue;

      int index = 0;
      for(; index < noSecs; ++index) if(buf[0x40+index] == hSec.r-1) break;
      if(index == noSecs) continue;

      for(int i = 0; i < sectorSize; ++i)
      {
        WORD blockNum = noBlksOnTrk*trk+sectorSize*index+i;
        if(blockNum >= noBlocks) break;
        blks[blockNum] = hSec.offset + i*blockSize;
      }
    }
  }
  closeFile();
}

Filer::~Filer()
{
  free(blks);
}

bool Filer::openFile(void)
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

bool Filer::closeFile(void)
{
  return CloseHandle(hostFile);
}

bool Filer::read(WORD blockNum, BYTE* buf)
{
  ZeroMemory(buf, blockSize);
  if(!blks[blockNum]) return false;
  SetFilePointer(hostFile, blks[blockNum], NULL, FILE_BEGIN);
  DWORD noBytesRead;
  ReadFile(hostFile, buf, blockSize, &noBytesRead, NULL);
  return (noBytesRead == blockSize);
}

bool Filer::write(WORD blockNum, BYTE* buf)
{
  if(!blks[blockNum]) return false;
  SetFilePointer(hostFile, blks[blockNum], NULL, FILE_BEGIN);
  DWORD noBytesWritten;
  WriteFile(hostFile, buf, blockSize, &noBytesWritten, NULL);
  return (noBytesWritten == blockSize);
}
