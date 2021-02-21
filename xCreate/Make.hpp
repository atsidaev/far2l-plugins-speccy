#ifndef MAKE_HPP
#define MAKE_HPP

#define FDDTRACKMAX (85*2)
#define FDDSECTMAX 30

#define GAP1Size 10
#define GAP2Size 22
#define GAP3Size 60
#define GAP4Size 192

struct FDITrk
{
  DWORD offset;
  WORD  reserved;
  BYTE  noSecs;
};

struct FDDSec
{
  BYTE  c;
  BYTE  h;
  BYTE  r;
  BYTE  n;
  DWORD sectPos;
};

struct TDHdr
{
  char id[2];
  BYTE volume;
  BYTE volumeId;
  BYTE version;
  BYTE den;
  BYTE drive;
  BYTE trk_den;
  BYTE dos;
  BYTE noSides;
  WORD crc;
};

struct UDIHdr
{
  BYTE  signature[4];
  DWORD fileSize;
  BYTE  version;
  BYTE  noCyls;
  BYTE  noHeads;
  BYTE  flag;
  BYTE  unuse[4];
};

struct TDInfo
{
  WORD crc;
  WORD textSize;
  BYTE year;
  BYTE month;
  BYTE day;
  BYTE hour;
  BYTE min;
  BYTE sec;
  char text[0x248];
};

bool createTRD(Track0 track0, int totalSecs, HANDLE image, HANDLE boot);
bool createFDI(Track0 track0, int totalSecs, BYTE *interleave, BYTE writeProtect, HANDLE image, HANDLE boot, char *comment);
bool createSCL(Track0 track0, int totalSecs, HANDLE image, HANDLE boot);
bool createFDD(Track0 track0, int totalSecs, BYTE *interleave, HANDLE image, HANDLE boot);
bool createUDI(Track0 track0, int totalSecs, BYTE *interleave, HANDLE image, HANDLE boot, char *comment);
bool createTD (Track0 track0, int totalSecs, BYTE *interleave, HANDLE image, HANDLE boot, char *comment);

#endif