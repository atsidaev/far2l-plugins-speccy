#include <windows.h>
#include "trdos.hpp"
#include "make.hpp"

bool createTRD(Track0 track0, int totalSecs, HANDLE image, HANDLE boot)
{
  int i;
  DWORD noBytesWritten;
  DWORD noBytesRead;

  BYTE emptySec[secSize];
  ZeroMemory(emptySec, secSize);

  //дорожка 0
  SetFilePointer(image, 0, NULL, FILE_BEGIN);
  WriteFile(image, &track0, sizeof(Track0), &noBytesWritten, NULL);

  //файлы
  for(i = 0; i < totalSecs; ++i)
  {
    BYTE buf[secSize];
    ReadFile(boot, buf, secSize, &noBytesRead, NULL);
    WriteFile(image, buf, secSize, &noBytesWritten, NULL);
  }

  //пустые сектора
  for(i = totalSecs; i < 2544; ++i)
    WriteFile(image, emptySec, secSize, &noBytesWritten, NULL);

  return true;
}
