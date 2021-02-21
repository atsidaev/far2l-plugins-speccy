#include <windows.h>
#include "trdos.hpp"
#include "make.hpp"
#include "tools.hpp"

bool createSCL(Track0 track0, int totalSecs, HANDLE image, HANDLE boot)
{
  int i;
  BYTE signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R'};
  DWORD checkSum = 0x255;

  DWORD noBytesWritten;
  DWORD noBytesRead;

  //�����䨪���
  WriteFile(image, signature, sizeof(signature), &noBytesWritten, NULL);

  //������⢮ 䠩���
  WriteFile(image, &track0.noFiles, 1, &noBytesWritten, NULL);
  checkSum += track0.noFiles;

  //��������� 䠩���
  for(i = 0; i < track0.noFiles; ++i)
  {
    WriteFile(image, &track0.files[i], sizeof(SCLFileHdr), &noBytesWritten, NULL);
    checkSum += calculateCheckSum((BYTE*)&track0.files[i], sizeof(SCLFileHdr));
  }

  //䠩��
  for(i = 0; i < totalSecs; ++i)
  {
    BYTE buf[secSize];
    ReadFile(boot, buf, secSize, &noBytesRead, NULL);
    WriteFile(image, buf, secSize, &noBytesWritten, NULL);
    checkSum += calculateCheckSum(buf, secSize);
  }

  //����஫쭠� �㬬�
  WriteFile(image, &checkSum, 4, &noBytesWritten, NULL);

  return true;
}
