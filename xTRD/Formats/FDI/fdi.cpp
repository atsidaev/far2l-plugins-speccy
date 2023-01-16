#include "filer.hpp"
#include "fdi.hpp"
#include "../../../shared/widestring.hpp"

bool fdi_isImage(const char* fileName, const BYTE* data, int size)
{
  if(data[0] != 'F' || data[1] != 'D' || data[2] != 'I') return false;

  HANDLE hostFile = CreateFile(_W((char*)fileName).c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return false;

  DWORD noBytesRead;
  FDIHdr hFDI;
  ReadFile(hostFile, &hFDI, sizeof(FDIHdr), &noBytesRead, 0);
  SetFilePointer(hostFile, sizeof(FDIHdr)+hFDI.extraInfoSize, NULL, FILE_BEGIN);

  TrackHdr hTrk;
  ReadFile(hostFile, &hTrk, sizeof(TrackHdr), &noBytesRead, 0);

  SectorHdr *hSecs = new SectorHdr[hTrk.noSecs];
  WORD mask = 0;
  WORD sysOffset = 0;
  
  ReadFile(hostFile, hSecs, sizeof(SectorHdr)*hTrk.noSecs, &noBytesRead, 0);
  for(BYTE sec = 0; sec < hTrk.noSecs; ++sec)
  {
    if(hSecs[sec].flags == 2 &&
       hSecs[sec].n     == 1 &&
       hSecs[sec].r     >  0 &&
       hSecs[sec].r     <  17) mask |= (1<<(hSecs[sec].r-1));
    if(hSecs[sec].flags == 2 &&
       hSecs[sec].n     == 1 &&
       hSecs[sec].r     == 9) sysOffset = hSecs[sec].offset;
  }
  
  delete[] hSecs;

  if(mask != 0xFFFF)
  { 
    CloseHandle(hostFile);
    return false;
  }

  BYTE signature = 0;
  SetFilePointer(hostFile, hFDI.dataOffset + hTrk.offset + sysOffset + 0xE7, NULL, FILE_BEGIN);

  ReadFile(hostFile, &signature, 1, &noBytesRead, NULL);
  CloseHandle(hostFile);
  
  if(signature != 0x10) return false;

  return true;
}

HANDLE fdi_init(const char* fileName)
{
  return (HANDLE)(new FilerFDI(fileName));
}

void fdi_cleanup(HANDLE h)
{
  delete (FilerFDI*)h;
}

bool fdi_reload (HANDLE h) { return true; }

bool fdi_open(HANDLE h)
{
  FilerFDI* f = (FilerFDI*)h;
  return f->open();
}

bool fdi_close(HANDLE h)
{
  FilerFDI* f = (FilerFDI*)h;
  return f->close();
}

bool fdi_read(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  FilerFDI* f = (FilerFDI*)h;
  return f->read(trk, sec, buf);
}

bool fdi_write(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  FilerFDI* f = (FilerFDI*)h;
  return f->write(trk, sec, buf);
}

char* fdi_getFormatName (void)
{
  static char* name = "FDI";
  return name;
}

bool fdi_isProtected(HANDLE h)
{
  FilerFDI* f = (FilerFDI*)h;
  return f->isProtected();
}

bool fdi_protect(HANDLE h, bool on)
{
  FilerFDI* f = (FilerFDI*)h;
  return f->protect(on);
}
