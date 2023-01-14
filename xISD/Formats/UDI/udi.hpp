#ifndef udi_hpp
#define udi_hpp

const unsigned short blockSize = 0x100;

struct UDIHdr
{
  char  signature[4];
  DWORD fileSize;
  BYTE  version;
  BYTE  noCyls;
  BYTE  noHeads;
  BYTE  flags;
  DWORD extraInfoSize;
};

struct SectorHdr
{
  BYTE c, h, r, n;
};

struct TrackHdr
{
  BYTE type;
  WORD tLen;
};

BYTE readCByte (BYTE *ctr, WORD byte)
{
  return ( ctr[byte>>3] & (1<<(byte%8)) );
};

#endif
