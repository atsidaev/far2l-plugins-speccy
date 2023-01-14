#ifndef teledisk_hpp
#define teledisk_hpp

const int blockSize = 0x100;

struct ImageHdr
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

struct ImageInfo
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

struct TrkHdr
{
  BYTE noSecs;
  BYTE cyl;
  BYTE head;
  BYTE crc;
};

struct SecHdr
{
  BYTE c;
  BYTE h;
  BYTE r;
  BYTE n;
  BYTE flag;
  BYTE crc;
};

#endif
