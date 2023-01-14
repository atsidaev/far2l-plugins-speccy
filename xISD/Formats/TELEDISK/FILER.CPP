#include <windows.h>
#include "filer.hpp"
#include "teledisk.hpp"
#include "td_tools.hpp"

Filer::Filer(char* fileName)
{
  lstrcpy(fName, fileName);
  imageBuf = NULL;
  masks = NULL;
}

Filer::~Filer()
{
  if(imageBuf)
    free(imageBuf);
  if(masks)
    free(masks);
}

bool Filer::reload(void)
{
  openFile();

  BYTE buf[0x4002];
  BYTE packedSec[0x4002];
  BYTE sectorType = 255;
  BYTE noSecs;
  BYTE noTrks;
  WORD track = 0;

  DWORD noBytesRead;

  ImageHdr imgHdr;
  TrkHdr trkHdr;
  SecHdr secHdr;
  int sec;

  ReadFile(hostFile, &imgHdr, sizeof(ImageHdr), &noBytesRead, NULL);

  bool isPacked = false;
  if(imgHdr.id[0] == 't' && imgHdr.id[1] == 'd')
    isPacked = true;
  td_init(hostFile, fName, imgHdr, isPacked);

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

  sectorSize = buf[0x18];
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

  noSecs   = buf[0x19];
  noBlocks = *(WORD*)(buf+0x12);

  if(imageBuf)
    free(imageBuf);
  if(masks)
    free(masks);
  imageBuf = (BYTE*)malloc(noBlocks*blockSize);
  ZeroMemory(imageBuf, noBlocks*blockSize);

  noTrks = noBlocks/(noSecs*sectorSize);

  masks = (bool *)malloc(sizeof(bool) * noBlocks / sectorSize);
  ZeroMemory(masks, sizeof(bool) * noBlocks / sectorSize);

  SetFilePointer(hostFile, sizeof(ImageHdr), NULL, FILE_BEGIN);

  td_init(hostFile, fName, imgHdr, isPacked);

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
    for(sec = 0; sec < trkHdr.noSecs; ++sec)
    {
      read_((BYTE*)&secHdr, sizeof(SecHdr));

      if((!(secHdr.n & 0xF8)) != (!(secHdr.flag & 0x30))) continue;

      short secSize;
      read_((BYTE*)&secSize, sizeof(secSize));
      WORD trkNum = trkHdr.cyl<<1 | trkHdr.head;
      read_(packedSec, secSize);

      if(secHdr.n != sectorType) continue;

      int index = 0;
      for(; index < noSecs; ++index) if(buf[0x40+index] == secHdr.r-1) break;
      if(index == noSecs) continue;

      if((sectorSize + noSecs * trkNum + index) >= noBlocks)
        goto error;

      BYTE* secBuf = imageBuf + blockSize * sectorSize * (noSecs * trkNum + index);

      if(!unRLE(packedSec, secSize, secBuf)) goto error;
      if(secHdr.crc != (BYTE)calculateCRC16(secBuf, blockSize*sectorSize)) goto error;
      masks[trkNum*noSecs + index] = true;
    }
    track++;
    if(track >= noTrks) break;
  }
  closeFile();
  return true;

error:

  closeFile();
  return false;
}

bool Filer::openFile(void)
{
  hostFile = CreateFile(fName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
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
  if(blockNum >= noBlocks) return false;
  if(!masks[blockNum/sectorSize]) return false;
  CopyMemory(buf, imageBuf + blockNum * blockSize, blockSize);
  return true;
}
