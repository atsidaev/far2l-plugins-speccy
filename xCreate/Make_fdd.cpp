#include <windows.h>
#include "trdos.hpp"
#include "make.hpp"

bool createFDD(Track0 track0, int totalSecs, BYTE *interleave, HANDLE image, HANDLE boot)
{
  int i,j;
  BYTE trkHdr[] = { 0, noSecs };

  DWORD noBytesWritten;
  DWORD noBytesRead;

  BYTE emptySec[secSize];
  ZeroMemory(emptySec, secSize);

  //заголовок файла
  BYTE FDDHdr[] =
  {
    'S', 'P', 'M', ' ',
    'D', 'I', 'S', 'K',' ',
    '(', 'c', ')', ' ',
    '1', '9', '9','6', ' ',
    'M', 'O', 'A', ' ',
    'v', '0', '.', '1',
    ' ', ' ', ' ', ' ',   // сигнатура
    0x50,                 // число треков
    0x02,                 // число сторон
    0x00,0x00,0x00,0x00   // unused
  };
  WriteFile(image, FDDHdr, sizeof(FDDHdr), &noBytesWritten, NULL);

  //смещения в файле к структурам заголовков треков
  DWORD trkIdx = sizeof(FDDHdr) + FDDTRACKMAX * 4;
  for(i = 0; i < 2*noTrks; ++i)
  {
    DWORD trk_idx = trkIdx;
    WriteFile(image, &trk_idx, 4, &noBytesWritten, NULL);
    trkIdx += sizeof(trkHdr) + FDDSECTMAX * sizeof(FDDSec) + secSize * noSecs;
  }
  for(i = 0; i < (FDDTRACKMAX - 2 * noTrks); ++i)
  {
    DWORD trk_idx = 0;
    WriteFile(image, &trk_idx, 4, &noBytesWritten, NULL);
  }

  //дорожки
  int savedSecs = 0;
  for(i = 0; i < 2*noTrks; ++i)
  {
    WriteFile(image, trkHdr, sizeof(trkHdr), &noBytesWritten, NULL);

    for(j = 0; j < FDDSECTMAX; ++j)
    {
      FDDSec secHdr = { 0, 0, 0, 1, 0 };
      secHdr.c = i/2;
      secHdr.r = interleave[j];
      secHdr.sectPos = sizeof(FDDHdr) + FDDTRACKMAX * 4 +
           i * (sizeof(trkHdr) + FDDSECTMAX * sizeof(FDDSec) + secSize * noSecs) +
                sizeof(trkHdr) + FDDSECTMAX * sizeof(FDDSec) + secSize * (interleave[j] - 1);
      if(j < noSecs)
        WriteFile(image, &secHdr, sizeof(FDDSec), &noBytesWritten, NULL);
      else
        WriteFile(image, emptySec, sizeof(FDDSec), &noBytesWritten, NULL);
    }

    if(i == 0)
    {
      WriteFile(image, (BYTE*)&track0 , sizeof(Track0), &noBytesWritten, NULL);
      continue;
    }

    BYTE buf[secSize*noSecs];
    ZeroMemory(buf, noSecs*secSize);
    
    if(savedSecs < totalSecs)
    {
      if((totalSecs - savedSecs) < 16)
        ReadFile(boot, buf, (totalSecs - savedSecs)*secSize, &noBytesRead, NULL);
      else
        ReadFile(boot, buf, noSecs*secSize, &noBytesRead, NULL);
    }

    WriteFile(image, buf, noSecs*secSize, &noBytesWritten, NULL);
    savedSecs+=16;
  }

  return true;
}
