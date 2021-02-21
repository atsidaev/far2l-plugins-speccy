#include <windows.h>
#include "trdos.hpp"
#include "make.hpp"

WORD crcTab[256] = {0x0000, 0x97A0, 0xB9E1, 0x2E41, 0xE563, 0x72C3, 0x5C82, 0xCB22,
                    0xCAC7, 0x5D67, 0x7326, 0xE486, 0x2FA4, 0xB804, 0x9645, 0x01E5,
                    0x032F, 0x948F, 0xBACE, 0x2D6E, 0xE64C, 0x71EC, 0x5FAD, 0xC80D,
                    0xC9E8, 0x5E48, 0x7009, 0xE7A9, 0x2C8B, 0xBB2B, 0x956A, 0x02CA,
                    0x065E, 0x91FE, 0xBFBF, 0x281F, 0xE33D, 0x749D, 0x5ADC, 0xCD7C,
                    0xCC99, 0x5B39, 0x7578, 0xE2D8, 0x29FA, 0xBE5A, 0x901B, 0x07BB,
                    0x0571, 0x92D1, 0xBC90, 0x2B30, 0xE012, 0x77B2, 0x59F3, 0xCE53,
                    0xCFB6, 0x5816, 0x7657, 0xE1F7, 0x2AD5, 0xBD75, 0x9334, 0x0494,
                    0x0CBC, 0x9B1C, 0xB55D, 0x22FD, 0xE9DF, 0x7E7F, 0x503E, 0xC79E,
                    0xC67B, 0x51DB, 0x7F9A, 0xE83A, 0x2318, 0xB4B8, 0x9AF9, 0x0D59,
                    0x0F93, 0x9833, 0xB672, 0x21D2, 0xEAF0, 0x7D50, 0x5311, 0xC4B1,
                    0xC554, 0x52F4, 0x7CB5, 0xEB15, 0x2037, 0xB797, 0x99D6, 0x0E76,
                    0x0AE2, 0x9D42, 0xB303, 0x24A3, 0xEF81, 0x7821, 0x5660, 0xC1C0,
                    0xC025, 0x5785, 0x79C4, 0xEE64, 0x2546, 0xB2E6, 0x9CA7, 0x0B07,
                    0x09CD, 0x9E6D, 0xB02C, 0x278C, 0xECAE, 0x7B0E, 0x554F, 0xC2EF,
                    0xC30A, 0x54AA, 0x7AEB, 0xED4B, 0x2669, 0xB1C9, 0x9F88, 0x0828,
                    0x8FD8, 0x1878, 0x3639, 0xA199, 0x6ABB, 0xFD1B, 0xD35A, 0x44FA,
                    0x451F, 0xD2BF, 0xFCFE, 0x6B5E, 0xA07C, 0x37DC, 0x199D, 0x8E3D,
                    0x8CF7, 0x1B57, 0x3516, 0xA2B6, 0x6994, 0xFE34, 0xD075, 0x47D5,
                    0x4630, 0xD190, 0xFFD1, 0x6871, 0xA353, 0x34F3, 0x1AB2, 0x8D12,
                    0x8986, 0x1E26, 0x3067, 0xA7C7, 0x6CE5, 0xFB45, 0xD504, 0x42A4,
                    0x4341, 0xD4E1, 0xFAA0, 0x6D00, 0xA622, 0x3182, 0x1FC3, 0x8863,
                    0x8AA9, 0x1D09, 0x3348, 0xA4E8, 0x6FCA, 0xF86A, 0xD62B, 0x418B,
                    0x406E, 0xD7CE, 0xF98F, 0x6E2F, 0xA50D, 0x32AD, 0x1CEC, 0x8B4C,
                    0x8364, 0x14C4, 0x3A85, 0xAD25, 0x6607, 0xF1A7, 0xDFE6, 0x4846,
                    0x49A3, 0xDE03, 0xF042, 0x67E2, 0xACC0, 0x3B60, 0x1521, 0x8281,
                    0x804B, 0x17EB, 0x39AA, 0xAE0A, 0x6528, 0xF288, 0xDCC9, 0x4B69,
                    0x4A8C, 0xDD2C, 0xF36D, 0x64CD, 0xAFEF, 0x384F, 0x160E, 0x81AE,
                    0x853A, 0x129A, 0x3CDB, 0xAB7B, 0x6059, 0xF7F9, 0xD9B8, 0x4E18,
                    0x4FFD, 0xD85D, 0xF61C, 0x61BC, 0xAA9E, 0x3D3E, 0x137F, 0x84DF,
                    0x8615, 0x11B5, 0x3FF4, 0xA854, 0x6376, 0xF4D6, 0xDA97, 0x4D37,
                    0x4CD2, 0xDB72, 0xF533, 0x6293, 0xA9B1, 0x3E11, 0x1050, 0x87F0};

WORD calculateCRC16(BYTE* buf, WORD size)
{
  WORD crc = 0;
  while(size--) crc = (crc>>8) ^ crcTab[(crc&0xff) ^ *buf++];
  return ((crc&0xff)<<8) | (crc>>8);
}

bool createTD(Track0 track0, int totalSecs, BYTE *interleave, HANDLE image, HANDLE boot, char *comment)
{
  int i,j;

  DWORD noBytesWritten;
  DWORD noBytesRead;

  BYTE emptySec[secSize];
  ZeroMemory(emptySec, secSize);

  //��������� 䠩��
  TDHdr hdr =
  {
    'T', 'D', // id
    0,    // volume num
    0,    // volume id
    0x15, // TeleDisk version
    0,    // DD disk
    2,    // 1.2 Mb drive
    0,    // flags
    0,    // flags
    2     // side
  };
  if(comment)
    hdr.trk_den = 0x80;
  hdr.crc = calculateCRC16((BYTE*)&hdr, sizeof(TDHdr) - 2);
  WriteFile(image, &hdr, sizeof(TDHdr), &noBytesWritten, NULL);

  //���ᠭ��
  if(comment)
  {
    TDInfo info;
    SYSTEMTIME systemTime;
    GetSystemTime(&systemTime);
    info.year  = systemTime.wYear - 1900;
    info.month = systemTime.wMonth;
    info.day   = systemTime.wDay;
    info.hour  = systemTime.wHour;
    info.min   = systemTime.wMinute;
    info.sec   = systemTime.wSecond;
    info.textSize = lstrlen(comment) + 8;
    ZeroMemory(info.text, sizeof(info.text));
    CopyMemory(info.text, comment, lstrlen(comment));
    info.crc   = calculateCRC16((BYTE*)&info.textSize, info.textSize + 8);

    WriteFile(image, &info, info.textSize+10, &noBytesWritten, NULL);
  }

  //��஦��
  int savedSecs = 0;
  BYTE trkHdr[4];
  for(i = 0; i < 2*noTrks; ++i)
  {
    trkHdr[0] = noSecs;
    trkHdr[1] = i/2;
    trkHdr[2] = i&1;
    trkHdr[3] = calculateCRC16((BYTE*)&trkHdr, sizeof(trkHdr) - 1);
    WriteFile(image, trkHdr, sizeof(trkHdr), &noBytesWritten, NULL);

    BYTE buf[secSize*noSecs];
    
    if((savedSecs < totalSecs) && (i!=0))
    {
      if((totalSecs - savedSecs) < 16)
      {
        ZeroMemory(buf, noSecs*secSize);
        ReadFile(boot, buf, (totalSecs - savedSecs)*secSize, &noBytesRead, NULL);
      }
      else
        ReadFile(boot, buf, noSecs*secSize, &noBytesRead, NULL);
    }

    for(j = 0; j < noSecs; ++j)
    {
      BYTE secHdr[] = 
      {
        0, 0, 0, 1, // C, H, R, N (sector header)
        0,          // flags
        0,          // checksum
        1, 1,       // sector size
        0           // not packed
      };
      secHdr[0] = i/2;
      secHdr[2] = interleave[j];

      if(i == 0)
      {
        BYTE *ptr = (BYTE*)&track0 + secSize * (interleave[j] - 1);
        secHdr[5] = calculateCRC16(ptr, secSize);
        WriteFile(image, &secHdr, sizeof(secHdr), &noBytesWritten, NULL);
        WriteFile(image, ptr, secSize, &noBytesWritten, NULL);
        continue;
      }

      if(savedSecs < totalSecs)
      {
        secHdr[5] = calculateCRC16(buf + secSize * (interleave[j] - 1), secSize);
        WriteFile(image, &secHdr, sizeof(secHdr), &noBytesWritten, NULL);
        WriteFile(image, buf + secSize * (interleave[j] - 1), secSize, &noBytesWritten, NULL);
        if(((totalSecs - savedSecs) >= 16) || (((totalSecs - savedSecs) < 16) && (interleave[j] <= (totalSecs%16))))
          savedSecs++;
      }
      else
      {
        secHdr[5] = 0;
        WriteFile(image, &secHdr, sizeof(secHdr), &noBytesWritten, NULL);
        WriteFile(image, emptySec, secSize, &noBytesWritten, NULL);
      }
    }
  }

  //
  trkHdr[0] = 0xFF;
  trkHdr[1] = 0;
  trkHdr[2] = 0;
  trkHdr[3] = 0;
  WriteFile(image, trkHdr, sizeof(trkHdr), &noBytesWritten, NULL);

  return true;
}