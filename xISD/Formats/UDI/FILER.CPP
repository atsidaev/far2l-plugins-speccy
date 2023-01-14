#include <windows.h>
#include "filer.hpp"
#include "udi.hpp"

WORD wdcrc(BYTE *ptr, WORD size)
{
   DWORD crc1 = 0xCDB4;
   int j;
   while(size--)
   {
     crc1 ^= (*ptr++) << 8;
     for(j = 8; j; j--)
        if((crc1 *= 2l) & 0x10000)
          crc1 ^= 0x1021; // bit representation of x^12+x^5+1
   }
   return (WORD) (((crc1 & 0xFF00) >> 8) | ((crc1 & 0xFF) << 8));
}

Filer::Filer(char* fileName)
{
  lstrcpy(fName, fileName);
  openFile();

  DWORD noBytesRead;
  UDIHdr hUDI;
  ReadFile(hostFile, &hUDI, sizeof(UDIHdr), &noBytesRead, 0);
  
  SetFilePointer(hostFile, hUDI.extraInfoSize+sizeof(UDIHdr), NULL, FILE_BEGIN);
  TrackHdr hTrk;
  ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);

  BYTE *Trk, *cTrk;
  WORD cTLen;
  cTLen = hTrk.tLen/8 + ((hTrk.tLen-(hTrk.tLen/8)*8)? 1:0);
  Trk  = (BYTE*)malloc(hTrk.tLen);
  cTrk = (BYTE*)malloc(cTLen);
  ReadFile(hostFile, Trk, hTrk.tLen, &noBytesRead, 0);
  ReadFile(hostFile, cTrk, cTLen, &noBytesRead, 0);

  WORD i;
  SectorHdr *hSec;
  for(i = 0; i < (hTrk.tLen - blockSize - 14); i++)
  {  
    if(readCByte(cTrk,i) == 0) // search for ID AM
      continue;
    while(readCByte(cTrk,i)) i++;
    if(Trk[i] != 0xFE) // skip ID AM
      continue;
    i++;

    hSec = (SectorHdr *)&Trk[i];
    i += sizeof(SectorHdr);
    i += 2; // skip CRC of ID AM

    // continue work with GAP
    while(readCByte(cTrk,i) == 0) // skip GAP
      i++;
    while(readCByte(cTrk,i)) // skip DATA AM
      i++;
    if((Trk[i] != 0xFB) && (Trk[i] != 0xF8))
      {i -= 3; continue; }
    i++;

    // continue work only if DATA AM found after ID AM
    if(hSec->r == 1) break;
  }

  BYTE buf[blockSize];
  memcpy(buf,&Trk[i],blockSize);

  sectorSize = buf[0x18];
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
  free(Trk);
  free(cTrk);

  SetFilePointer(hostFile, hUDI.extraInfoSize+sizeof(UDIHdr), NULL, FILE_BEGIN);
  BYTE noBlksOnTrk = noSecs*sectorSize;
  DWORD offset = hUDI.extraInfoSize+sizeof(UDIHdr);

  for(WORD trk = 0; trk < (hUDI.noCyls+1)*(hUDI.noHeads+1); ++trk)
  {
    ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);
    offset+=sizeof(TrackHdr);

    cTLen = hTrk.tLen/8 + ((hTrk.tLen-(hTrk.tLen/8)*8)? 1:0);
    Trk = (BYTE*)malloc(hTrk.tLen);
    cTrk = (BYTE*)malloc(cTLen);
    ReadFile(hostFile, Trk, hTrk.tLen, &noBytesRead, 0);
    ReadFile(hostFile, cTrk, cTLen, &noBytesRead, 0);

    for(i = 0; i < (hTrk.tLen - sectorSize*blockSize - 14); i++)
    {  
      if(readCByte(cTrk,i) == 0) // search for ID AM
        continue;
      while(readCByte(cTrk,i)) i++;
      if(Trk[i] != 0xFE) // skip ID AM
        continue;
      i++;

      hSec = (SectorHdr *)&Trk[i];
      i += sizeof(SectorHdr);
      i += 2; // skip CRC of ID AM

      // continue work with GAP
      while(readCByte(cTrk,i) == 0) // skip GAP
        i++;
      while(readCByte(cTrk,i)) // skip DATA AM
        i++;
      if((Trk[i] != 0xFB) && (Trk[i] != 0xF8))
        { i-= 2; continue; }
      i++;

      // continue work only if DATA AM found after ID AM
      if(hSec->n != sectorType) continue;

      int index = 0;
      for(; index < noSecs; ++index) if(buf[0x40+index] == hSec->r-1) break;
      if(index == noSecs) continue;

      for(WORD j = 0; j < sectorSize; ++j)
      {
        WORD blockNum = noBlksOnTrk*trk + sectorSize*index + j;
        if(blockNum >= noBlocks) break;
        blks[blockNum] = offset + i + j*blockSize;
      }

      i += sectorSize*blockSize;
    }

    offset += (DWORD)hTrk.tLen;
    offset += (DWORD)cTLen;
    free(Trk);
    free(cTrk);
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
  isChanged = false;
  return (hostFile != INVALID_HANDLE_VALUE);
}

bool Filer::closeFile(void)
{
  if(isChanged)
  {
    isChanged = false;
    DWORD fileSize = GetFileSize(hostFile, NULL) - 4;

    HANDLE mh = CreateFileMapping(hostFile, 0, PAGE_READWRITE, 0, 0, NULL);
    if(mh == NULL) return CloseHandle(hostFile);

    void *buf = MapViewOfFile(mh, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    long CRC = 0xFFFFFFFF;
    for(DWORD i = 0; i < fileSize; i++)
    {
      DWORD temp;
      CRC ^= -1 ^ *(((unsigned char*)buf)+i);
      for(BYTE k = 8; k--;)
        { temp = -(CRC & 1); CRC >>= 1; CRC ^= 0xEDB88320 & temp; }
      CRC ^= -1;
    }

    BYTE *CRCptr = (BYTE*)buf + fileSize;
    *(DWORD*)CRCptr = CRC;

    UnmapViewOfFile(buf);
    CloseHandle(mh);
  }

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
  bool result;

  isChanged = true;

  SetFilePointer(hostFile, blks[blockNum], NULL, FILE_BEGIN);
  DWORD noBytes;
  WriteFile(hostFile, buf, blockSize, &noBytes, NULL);
  result = noBytes == blockSize;

  BYTE *sectorBuf;
  WORD realSectorSize = blockSize * sectorSize + 1;
  sectorBuf = (BYTE*)malloc(realSectorSize);

  SetFilePointer(hostFile, blks[blockNum - (blockNum % sectorSize)] - 1, NULL, FILE_BEGIN);
  ReadFile(hostFile, sectorBuf, realSectorSize, &noBytes, NULL);
  if(noBytes != realSectorSize) result = false;

  WORD crc;
  crc = wdcrc(sectorBuf, realSectorSize);
  WriteFile(hostFile, &crc, sizeof(WORD), &noBytes, NULL);
  if(noBytes != sizeof(WORD)) result = false;

  return result;
}
