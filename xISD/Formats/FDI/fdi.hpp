#ifndef fdi_hpp
#define fdi_hpp

const unsigned short blockSize = 0x100;

#pragma pack(1)

struct FDIHdr
{
  char signature[3];
  BYTE writeProtection;
  WORD noCyls;
  WORD noHeads;
  WORD textOffset;
  WORD dataOffset;
  WORD extraInfoSize;
};

struct SectorHdr
{
  BYTE c, h, r, n;
  BYTE flags;
  WORD offset;
};

struct TrackHdr
{
  DWORD offset;
  WORD  reserved;
  BYTE  noSecs;
};

#pragma pack()

#endif
