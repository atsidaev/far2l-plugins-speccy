#include "../fmt.hpp"
#include "filer.hpp"
#include "udi.hpp"
#include "../../widestring.hpp"

bool WINAPI _export isImage(const char* fileName, const BYTE* data, int size)
{
  if(data[0] != 'U' || data[1] != 'D' || data[2] != 'I' || data[3] != '!' ||
     data[8] != 0) return false;

  HANDLE hostFile = CreateFile(_W((char*)fileName).c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return false;

  DWORD noBytesRead;
  UDIHdr hUDI;
  ReadFile(hostFile, &hUDI, sizeof(UDIHdr), &noBytesRead, 0);
  SetFilePointer(hostFile, hUDI.extraInfoSize+sizeof(UDIHdr), NULL, FILE_BEGIN);

  TrackHdr hTrk;
  ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);

  BYTE *Trk, *cTrk;
  WORD cTLen;
  cTLen = hTrk.tLen/8 + ((hTrk.tLen-(hTrk.tLen/8)*8)? 1:0);
  Trk  = new BYTE[hTrk.tLen];
  cTrk = new BYTE[cTLen];
  ReadFile(hostFile, Trk, hTrk.tLen, &noBytesRead, 0);
  ReadFile(hostFile, cTrk, cTLen, &noBytesRead, 0);
  CloseHandle(hostFile);

  WORD i;
  SectorHdr *hSec;
  WORD mask = 0;
  WORD sysOffset = 0;

  for(i = 0; i < (hTrk.tLen-sectorSize-14); i++)
  {  
    if(readCByte(cTrk,i)==0) // search for ID AM
      continue;
    while(readCByte(cTrk,i)) i++;
    if(Trk[i] != 0xFE) // skip ID AM
      continue;
    i++;

    hSec = (SectorHdr *)&Trk[i];
    i += sizeof(SectorHdr);
    i += 2; // skip CRC of ID AM

    // continue work with GAP
    while(readCByte(cTrk,i)==0) // skip GAP
      i++;
    while(readCByte(cTrk,i)) // skip DATA AM
      i++;
    if((Trk[i] != 0xFB) && (Trk[i] != 0xF8))
      {i -= 2; continue; }
    i++;

    // continue work only if DATA AM found after ID AM
    if(hSec->n == 1 &&
       hSec->r >  0 &&
       hSec->r <  17) mask |= (1<<(hSec->r-1));
    if(hSec->r == 9 &&
       hSec->n == 1) sysOffset = i;
  }

  if((mask == 0xFFFF) && (Trk[sysOffset+0xE7] == 0x10))
  { 
    delete[] cTrk;
    delete[] Trk;
    return true;
  }

  delete[] cTrk;
  delete[] Trk;
  return false;
}

HANDLE WINAPI _export init(const char* fileName)
{
  return (HANDLE)(new FilerUDI(fileName));
}

void WINAPI _export cleanup(HANDLE h)
{
  delete (FilerUDI*)h;
}

bool WINAPI _export reload (HANDLE h) { return true; }

bool WINAPI _export open(HANDLE h)
{
  FilerUDI* f = (FilerUDI*)h;
  return f->open();
}

bool WINAPI _export close(HANDLE h)
{
  FilerUDI* f = (FilerUDI*)h;
  return f->close();
}

bool WINAPI _export read(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  FilerUDI* f = (FilerUDI*)h;
  return f->read(trk, sec, buf);
}

bool WINAPI _export write(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  FilerUDI* f = (FilerUDI*)h;
  return f->write(trk, sec, buf);
}

char* WINAPI _export getFormatName (void)
{
  static char* name = "UDI";
  return name;
}

bool WINAPI _export isProtected(HANDLE h)
{
  FilerUDI* f = (FilerUDI*)h;
  return f->isProtected();
}

bool WINAPI _export protect(HANDLE h, bool on)
{
  FilerUDI* f = (FilerUDI*)h;
  return f->protect(on);
}
