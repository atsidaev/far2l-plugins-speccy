#include "filer.hpp"
#include "fdi.hpp"

#include <windows.h>
#include "../../../shared/widestring.hpp"

bool WINAPI _export fdi_isImage(char* fileName, const BYTE* data, int size)
{
  if(data[0] != 'F' || data[1] != 'D' || data[2] != 'I') return false;

  HANDLE hostFile = CreateFile(_W(fileName).c_str(),
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

  int sec;
  SectorHdr hSec;
  for(sec = 0; sec < hTrk.noSecs; ++sec)
  {
    ReadFile(hostFile, &hSec, sizeof(SectorHdr), &noBytesRead, 0);
    if((hSec.r == 1) && (hSec.n > 0)) break;
  }

  if(sec == hTrk.noSecs)
  {
    CloseHandle(hostFile);
    return false;
  }

  SetFilePointer(hostFile, hFDI.dataOffset + hTrk.offset + hSec.offset, NULL, FILE_BEGIN);

  BYTE buf[blockSize];
  ReadFile(hostFile, buf, blockSize, &noBytesRead, 0);
  CloseHandle(hostFile);

  if((buf[10] != 'D' || buf[11] != 'S' || buf[12] != 'K') &&
     (buf[13] != 'D' || buf[14] != 'S' || buf[15] != 'K')) return false;
  return true;
}

HANDLE WINAPI _export fdi_openSubPlugin(char* fileName)
{
  return (HANDLE)(new Filer(fileName));
}

void WINAPI _export fdi_closeSubPlugin(HANDLE h)
{
  delete (Filer*)h;
}

bool WINAPI _export fdi_reload (HANDLE h) { return true; }

bool WINAPI _export fdi_openFile(HANDLE h)
{
  Filer* f = (Filer*)h;
  return f->openFile();
}

bool WINAPI _export fdi_closeFile(HANDLE h)
{
  Filer* f = (Filer*)h;
  return f->closeFile();
}

bool WINAPI _export fdi_read(HANDLE h, WORD blockNum, BYTE* buf)
{
  Filer* f = (Filer*)h;
  return f->read(blockNum, buf);
}

bool WINAPI _export fdi_write(HANDLE h, WORD blockNum, BYTE* buf)
{
  Filer* f = (Filer*)h;
  return f->write(blockNum, buf);
}

char* WINAPI _export fdi_getFormatName(void)
{
  static char* name = "FDI";
  return name;
}
