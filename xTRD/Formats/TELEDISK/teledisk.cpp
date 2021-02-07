#include "../fmt.hpp"
#include "filer.hpp"
#include "teledisk.hpp"
#include "td_tools.hpp"
#include "../../../shared/widestring.hpp"

bool WINAPI _export td_isImage(const char* fileName, const BYTE* data, int size)
{
  if((data[0] != 't' || data[1] != 'd' || data[4] < 20) &&
     (data[0] != 'T' || data[1] != 'D'))
    return false;

  HANDLE hostFile = CreateFile(_W((char*)fileName).c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return false;

  WORD mask = 0;
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

    if(secHdr.n == 1 &&
       secHdr.r >  0 &&
       secHdr.r <  17) mask |= (1<<(secHdr.r-1));
    if((secHdr.r != 9) || (secHdr.n != 1)) continue;

    if(!unRLE(packedSec, secSize, buf)) goto error;
    if(secHdr.crc != (BYTE)calculateCRC16(buf, (128<<secHdr.n))) goto error;
  }

  if((mask == 0xFFFF) && (buf[0xE7] == 0x10))
  {
    CloseHandle(hostFile);
    return true;
  }

error:
  CloseHandle(hostFile);
  return false;
}

HANDLE WINAPI _export td_init(const char* fileName)
{
  return (HANDLE)(new FilerTD(fileName));
}

void WINAPI _export td_cleanup(HANDLE h)
{
  delete (FilerTD*)h;
}

bool WINAPI _export td_reload(HANDLE h)
{
  FilerTD* f = (FilerTD*)h;
  return f->reload();
}

bool WINAPI _export td_open(HANDLE h)
{
  FilerTD* f = (FilerTD*)h;
  return f->open();
}

bool WINAPI _export td_close(HANDLE h)
{
  FilerTD* f = (FilerTD*)h;
  return f->close();
}

bool WINAPI _export td_read(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  FilerTD* f = (FilerTD*)h;
  return f->read(trk, sec, buf);
}

bool WINAPI _export td_write(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  return false;
}

char* WINAPI _export td_getFormatName (void)
{
  static char* name = "TeleDisk";
  return name;
}

bool WINAPI _export td_isProtected(HANDLE h)
{
  return true;
}

bool WINAPI _export td_protect(HANDLE h, bool on)
{
  return false;
}
