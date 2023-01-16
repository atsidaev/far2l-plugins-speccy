#ifndef iSDOS_hpp
#define iSDOS_hpp
#include "types.hpp"

#pragma pack(1)

struct FileHdr
{
  u16 loadAddr;
  u8  size[3];
  u16 firstBlock;
  u8  systemFlag;
  u8  reserved[6];
  u16 crc;
  u16 time;
};

struct DirHdr
{
  u16 parentDir1stBlock;
  u16 size;
  u8  level;
  u16 descr1stBlock;
  u16 firstBlock;
  u8  totalFiles;
  u8  noFiles;
  u8  levelChic;
  u8  reserved[6];
};

struct UniHdr
{
  u8  name[8];
  u8  ext [3];
  u8  attr;
  union
  {
    FileHdr file;
    DirHdr  dir;
  };
  u16 date;
};


struct DiskHdr
{
  u8      reserved1[2];
  u8      title[8];
  u8      signature[3];
  u8      signatureChic[3];
  u8      reserved2[2];
  u16     noBlocks;
  u16     mainDir1stBlock;
  u8      noTrks;
  u8      type;
  u8      sectorSize;
  u8      noSecsOnTrk;
  u8      reserved3[4];
  u16     date;
  FileHdr is_dos_sys;
  u8      secTable[0x10];
};

#pragma pack()

const u8 FLAG_EXIST          = 0x01;
const u8 FLAG_READ_PROTECT   = 0x04;
const u8 FLAG_WRITE_PROTECT  = 0x08;
const u8 FLAG_HIDDEN         = 0x10;
const u8 FLAG_DIR            = 0x20;
const u8 FLAG_SOLID          = 0x40;
const u8 FLAG_DELETE_PROTECT = 0x80;

#endif
