#include <windows.h>
#include <fstream>
#include "filer.hpp"
#include "udi.hpp"
#include "../../../shared/widestring.hpp" 

WORD wdcrc(unsigned char *ptr, WORD size, BYTE dataId)
{
   DWORD crc1 = 0xCDB4;
   int j;
   crc1 ^= dataId << 8;
   for(j = 8; j; j--)
      if((crc1 *= 2l) & 0x10000)
        crc1 ^= 0x1021; // bit representation of x^12+x^5+1
   while(size--)
   {
     crc1 ^= (*ptr++) << 8;
     for(j = 8; j; j--)
        if((crc1 *= 2l) & 0x10000)
          crc1 ^= 0x1021; // bit representation of x^12+x^5+1
   }
   return (WORD) (((crc1 & 0xFF00) >> 8) | ((crc1 & 0xFF) << 8));
}

FilerUDI::FilerUDI(const char* fileName)
{
  strcpy(fName, fileName);
  open();

  DWORD noBytesRead;
  UDIHdr hUDI;
  ReadFile(hostFile, &hUDI, sizeof(UDIHdr), &noBytesRead, 0);
  
  maxSec = 16*(hUDI.noCyls+1)*(hUDI.noHeads+1);
  secs = new DWORD[maxSec];
  memset(secs, 0, maxSec);
  
  DWORD offset = hUDI.extraInfoSize+sizeof(UDIHdr);
  SetFilePointer(hostFile, sizeof(UDIHdr)+hUDI.extraInfoSize, NULL, FILE_BEGIN);
  
  for(int trk = 0; trk < (hUDI.noCyls+1)*(hUDI.noHeads+1); ++trk)
  {
    TrackHdr hTrk;
    ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);
    offset+=sizeof(TrackHdr);

    BYTE *Trk, *cTrk;
    WORD cTLen;
    cTLen = hTrk.tLen/8 + ((hTrk.tLen-(hTrk.tLen/8)*8)? 1:0);
    Trk  = new BYTE[hTrk.tLen];
    cTrk = new BYTE[cTLen];
    ReadFile(hostFile, Trk, hTrk.tLen, &noBytesRead, 0);
    ReadFile(hostFile, cTrk, cTLen, &noBytesRead, 0);

    for(WORD i = 0; i < (hTrk.tLen - sectorSize - 14); i++)
    {  
      if(readCByte(cTrk,i) == 0) // search for ID AM
        continue;
      while(readCByte(cTrk,i)) i++;
      if(Trk[i] != 0xFE) // skip ID AM
        continue;
      i++;

      SectorHdr *hSec = (SectorHdr *)&Trk[i];
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
      if((hSec->n != 1) || (hSec->r < 1) || (hSec->r > 16)) continue;

      secs[16*trk+hSec->r-1] = offset + i;
    }

    offset += (DWORD)hTrk.tLen;
    offset += (DWORD)cTLen;
    delete[] Trk;
    delete[] cTrk;
  }
  close();
}

FilerUDI::~FilerUDI()
{
  delete[] secs;
}

bool FilerUDI::open(void)
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
  isChanged = false;
  return (hostFile != INVALID_HANDLE_VALUE);
}

bool FilerUDI::close(void)
{
  DWORD fileSize = GetFileSize(hostFile, NULL) - 4;
  if (!CloseHandle(hostFile))
    return false;

  if(isChanged)
  {
    isChanged = false;

    std::fstream file(fName, std::ios_base::out | std::ios_base::in | std::ios_base::binary);
    if (!file.good())
      return false;

    int32_t CRC = 0xFFFFFFFF;
    unsigned char value;
    for(DWORD i = 0; i < fileSize; i++)
    {
      DWORD temp;
      file.read(reinterpret_cast<char*>(&value), 1);
      CRC ^= -1 ^ value;
      for(BYTE k = 8; k--;)
        { temp = -(CRC & 1); CRC >>= 1; CRC ^= 0xEDB88320 & temp; }
      CRC ^= -1;
    }
    
    file.write(reinterpret_cast<char*>(&CRC), sizeof(CRC));
    file.close();
  }

  return true;
}

bool FilerUDI::read(BYTE trk, BYTE sec, BYTE* buf)
{
  memset(buf, 0, sectorSize);
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  SetFilePointer(hostFile, secs[16*trk+sec], NULL, FILE_BEGIN);
  DWORD noBytesRead;
  ReadFile(hostFile, buf, sectorSize, &noBytesRead, NULL);
  return (noBytesRead == sectorSize);
}

bool FilerUDI::write(BYTE trk, BYTE sec, BYTE* buf)
{
  if(!secs[16*trk+sec] || 16*trk+sec >= maxSec) return false;
  bool result;
  isChanged = true;
  DWORD noBytes;

  BYTE id;
  SetFilePointer(hostFile, secs[16*trk+sec]-1, NULL, FILE_BEGIN);
  ReadFile(hostFile, &id, 1, &noBytes, NULL);
  result = noBytes == 1;

  WriteFile(hostFile, buf, sectorSize, &noBytes, NULL);
  if(noBytes != sectorSize) result = false;

  WORD crc;
  crc = wdcrc(buf, sectorSize, id);
  WriteFile(hostFile, &crc, sizeof(WORD), &noBytes, NULL);
  if(noBytes != sizeof(WORD)) result = false;

  return (result);
}

bool FilerUDI::isProtected(void)
{
  DWORD attr = GetFileAttributes(_W(fName).c_str());
  return (attr & FILE_ATTRIBUTE_READONLY);
}

bool FilerUDI::protect(bool on)
{
  DWORD attr = GetFileAttributes(_W(fName).c_str());
  if(on)
    attr |= FILE_ATTRIBUTE_READONLY;
  else
    attr &= ~FILE_ATTRIBUTE_READONLY;

  return (SetFileAttributes(_W(fName).c_str(), attr));
}
