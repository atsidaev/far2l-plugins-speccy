#include <windows.h>
#include "filer.hpp"
#include "teledisk.hpp"
#include "td_tools.hpp"
#include "../../../shared/widestring.hpp"

FilerTD::FilerTD(const char* fileName)
{
  strcpy(fName, fileName);
  imageBuf = new BYTE[166*16*sectorSize];
  memset(masks, 0, sizeof(masks));
}

FilerTD::~FilerTD()
{
  delete[] imageBuf;
}

bool FilerTD::reload(void)
{
  open();
  
  memset(masks, 0, sizeof(masks));

  ImageHdr imgHdr;
  
  DWORD noBytesRead;
  ReadFile(hostFile, &imgHdr, sizeof(ImageHdr), &noBytesRead, NULL);

  if(imgHdr.id[0] == 't' && imgHdr.id[1] == 'd')
    td_init(hostFile, fName, imgHdr, true);
  else
    td_init(hostFile, fName, imgHdr, false);
  
  int noTrks = 0;
  
  if(imgHdr.trk_den & 0x80)
  {
    ImageInfo info;
    read_((BYTE*)&info, 10);
    read_((BYTE*)info.text, info.textSize);
    if(info.crc != calculateCRC16((BYTE*)&info.textSize, info.textSize+8)) goto error;
  }
  
  while(1)
  {
    TrkHdr trkHdr;
    read_((BYTE*)&trkHdr, sizeof(TrkHdr));
    if(trkHdr.noSecs == 0xFF) break;
    if(trkHdr.crc != (BYTE)calculateCRC16((BYTE*)&trkHdr, sizeof(TrkHdr)-1)) goto error;
    for(int s = trkHdr.noSecs; s > 0; --s)
    {
      SecHdr secHdr;
      read_((BYTE*)&secHdr, sizeof(SecHdr));
      BYTE crc = calculateCRC16((BYTE*)&secHdr, sizeof(SecHdr)-1);

      if((!(secHdr.n & 0xF8)) == (!(secHdr.flag & 0x30)))
      {
        short secSize;
        read_((BYTE*)&secSize, sizeof(secSize));
        BYTE packedSec[0x4002];
        WORD trkNum = trkHdr.cyl<<1 | trkHdr.head;
        read_(packedSec, secSize);
        BYTE* sec = imageBuf + sectorSize*(16*trkNum + secHdr.r-1);
        
        if(secHdr.r > 0 && secHdr.r < 17 && secHdr.n == 1)
        {
          if(!unRLE(packedSec, secSize, sec)) goto error;
          crc = calculateCRC16(sec, 256);
          if(secHdr.crc != crc) goto error;
          masks[trkNum] |= 1<<(secHdr.r-1);
        }
        continue;
      }
      if(secHdr.crc != crc) goto error;
    }
    noTrks++;
    if(noTrks >= 166) break;
  }
  close();
  return true;

error:

  close();
  return false;
}

bool FilerTD::open(void)
{
  hostFile = CreateFile(_W(fName).c_str(),
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);
  return (hostFile != INVALID_HANDLE_VALUE);
}

bool FilerTD::close(void)
{
  return CloseHandle(hostFile);
}

bool FilerTD::read(BYTE trk, BYTE sec, BYTE* buf)
{
  memset(buf, 0, sectorSize);
  if(!(masks[trk] & 1<<sec)) return false;
  memcpy(buf, imageBuf + sectorSize*(16*trk + sec), sectorSize);
  return true;
}
