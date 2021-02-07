#ifndef fdi_hpp
#define fdi_hpp

const unsigned short sectorSize = 0x100;

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

#endif
