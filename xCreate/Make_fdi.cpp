#include <windows.h>
#include "trdos.hpp"
#include "make.hpp"

bool createFDI(Track0 track0, int totalSecs, BYTE *interleave, BYTE writeProtect, HANDLE image, HANDLE boot, char *comment)
{
  int i,j;
  DWORD noBytesWritten;
  DWORD noBytesRead;

  BYTE emptySec[secSize];
  memset(emptySec, 0, secSize);

  //заголовок файла
  BYTE FDIHdr[] =
  {
    'F', 'D', 'I',        // сигнатура
    0x00,                 // Флаг защиты записи  (0 - write enabled, 1 - write disabled)
    0x50, 0x00,           // число треков (80)
    0x02, 0x00,           // число сторон
    0x6E, 0x4A,           // Смещение текста (короткий комментарий к диску)
    0x00, 0x4B,           // Смещение данных
    0x00, 0x00            // Длина дополнительной информации в заголовке
  };
  FDIHdr[3] = writeProtect;
  WriteFile(image, FDIHdr, sizeof(FDIHdr), &noBytesWritten, NULL);

  //заголовки секторов
  for(i = 0; i < 2*noTrks; ++i)
  {
    FDITrk trk;
    trk.offset   = secSize * noSecs * i;
    trk.reserved = 0;
    trk.noSecs   = noSecs;
    WriteFile(image, &trk, sizeof(FDITrk), &noBytesWritten, NULL);
    for(j = 0; j < noSecs; ++j)
    {
      BYTE secHdr[7] = { 0, 0, 0, 1, 2, 0, 0 };
      secHdr[0] = i/2;
      secHdr[2] = interleave[j];
      secHdr[6] = interleave[j] - 1;
      WriteFile(image, secHdr, sizeof(secHdr), &noBytesWritten, NULL);
    }
  }

  //комментарий
  if(comment)
  {
    WriteFile(image, comment, strlen(comment), &noBytesWritten, NULL);
    WriteFile(image, emptySec, 146-strlen(comment), &noBytesWritten, NULL);
  }
  else
    WriteFile(image, emptySec, 146, &noBytesWritten, NULL);

  //дорожка 0
  SetFilePointer(image, 0x4B00, NULL, FILE_BEGIN);
  WriteFile(image, &track0, sizeof(Track0), &noBytesWritten, NULL);

  //файлы
  for(i = 0; i < totalSecs; ++i)
  {
    BYTE buf[secSize];
    ReadFile(boot, buf, secSize, &noBytesRead, NULL);
    WriteFile(image, buf, secSize, &noBytesWritten, NULL);
  }

  //пустые дорожки
  for(i = totalSecs; i < 2544; ++i)
    WriteFile(image, emptySec, secSize, &noBytesWritten, NULL);

  return true;
}
