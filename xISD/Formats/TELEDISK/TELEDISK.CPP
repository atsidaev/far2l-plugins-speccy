#include "..\fmt.hpp"
#include "filer.hpp"
#include "teledisk.hpp"
#include "td_tools.hpp"

bool WINAPI _export isImage(char* fileName, const BYTE* data, int size)
{
  if((data[0] != 't' || data[1] != 'd' || data[4] < 20) &&
     (data[0] != 'T' || data[1] != 'D'))
    return false;

  HANDLE hostFile = CreateFile(fileName,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return false;

  DWORD noBytesRead;
  ImageHdr imgHdr;

  ReadFile(hostFile, &imgHdr, sizeof(ImageHdr), &noBytesRead, NULL);
  if(imgHdr.crc != calculateCRC16((BYTE*)&imgHdr, sizeof(ImageHdr)-2) ||
     imgHdr.version < 10 || imgHdr.version > 21) goto error;

  while(1)
  {
    if(imgHdr.id[0] == 't' && imgHdr.id[1] == 'd' && imgHdr.version >= 20) break;
    if(imgHdr.id[0] == 'T' && imgHdr.id[1] == 'D') break;
    goto error;
  }

  BYTE buf[0x4002];
  BYTE packedSec[0x4002];

  TrkHdr   trkHdr;
  SecHdr   secHdr;
  int sec;

  if(imgHdr.id[0] == 't' && imgHdr.id[1] == 'd')
    td_init(hostFile, fileName, imgHdr, true);
  else
    td_init(hostFile, fileName, imgHdr, false);

  if(imgHdr.trk_den & 0x80)
  {
    ImageInfo info;
    read_((BYTE*)&info, 10);
    read_((BYTE*)info.text, info.textSize);
    if(info.crc != calculateCRC16((BYTE*)&info.textSize, info.textSize+8)) goto error;
  }

  read_((BYTE*)&trkHdr, sizeof(TrkHdr));
  if(trkHdr.noSecs == 0xFF) goto error;
  if(trkHdr.crc != (BYTE)calculateCRC16((BYTE*)&trkHdr, sizeof(TrkHdr)-1)) goto error;

  for(sec = 0; sec < trkHdr.noSecs; ++sec)
  {
    read_((BYTE*)&secHdr, sizeof(SecHdr));

    if((!(secHdr.n & 0xF8)) != (!(secHdr.flag & 0x30))) continue;

    short secSize;
    read_((BYTE*)&secSize, sizeof(secSize));
    read_(packedSec, secSize);

    if((secHdr.r != 1) || (secHdr.n == 0)) continue;

    if(!unRLE(packedSec, secSize, buf)) goto error;
    if(secHdr.crc != (BYTE)calculateCRC16(buf, (128<<secHdr.n))) goto error;
    break;
  }

  if(secHdr.r != 1 || secHdr.n == 0) goto error;

  if((buf[10] != 'D' || buf[11] != 'S' || buf[12] != 'K') &&
     (buf[13] != 'D' || buf[14] != 'S' || buf[15] != 'K')) goto error;

  CloseHandle(hostFile);
  return true;

error:
  CloseHandle(hostFile);
  return false;
}

HANDLE WINAPI _export openSubPlugin(char* fileName)
{
  return (HANDLE)(new Filer(fileName));
}

void WINAPI _export closeSubPlugin(HANDLE h)
{
  delete (Filer*)h;
}

bool WINAPI _export reload(HANDLE h)
{
  Filer* f = (Filer*)h;
  return f->reload();
}

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
  return false;
}

char* WINAPI _export getFormatName (void)
{
  static char* name = "TeleDisk";
  return name;
}
