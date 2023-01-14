#include "..\fmt.hpp"
#include "filer.hpp"
#include "udi.hpp"

bool WINAPI _export isImage(char* fileName, const BYTE* data, int size)
{
  if(data[0] != 'U' || data[1] != 'D' || data[2] != 'I' || data[3] != '!' ||
     data[8] != 0) return false;

  HANDLE hostFile = CreateFile(fileName,
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
  Trk  = (BYTE*)malloc(hTrk.tLen);
  cTrk = (BYTE*)malloc(cTLen);
  ReadFile(hostFile, Trk, hTrk.tLen, &noBytesRead, 0);
  ReadFile(hostFile, cTrk, cTLen, &noBytesRead, 0);
  CloseHandle(hostFile);

  WORD i;
  SectorHdr *hSec;
  for(i = 0; i < (hTrk.tLen-blockSize-14); i++)
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
    if((hSec->r == 1) && (hSec->n > 0)) break;
  }

  if((hSec->r != 1) || (hSec->n < 1)) return false;

  if((Trk[i+10] != 'D' || Trk[i+11] != 'S' || Trk[i+12] != 'K') &&
     (Trk[i+13] != 'D' || Trk[i+14] != 'S' || Trk[i+15] != 'K'))
  {
    free(Trk);
    free(cTrk);
    return false;
  }

  free(Trk);
  free(cTrk);
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
  static char* name = "UDI";
  return name;
}
