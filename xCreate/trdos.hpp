#ifndef TRDOS_HPP
#define TRDOS_HPP
#include <windows.h>

const int noTrks  = 80;  // число треков (целиндров) на одной стророне диска
const int noSecs  = 16;  // число секторов не треке
const int secSize = 256; // размер сектора в байтах

struct FileHdr
{
  char name[8];
  char type;
  WORD start;
  WORD length;
  BYTE noSecs;
  BYTE sec;
  BYTE trk;
};

struct Track0
{
  FileHdr files[142];
  BYTE    reserved1;
  BYTE    firstFreeSec;
  BYTE    firstFreeTrk;
  BYTE    type;
  BYTE    noFiles;
  WORD    noFreeSecs;
  BYTE    flag;
  BYTE    reserved2[12];
  BYTE    noDelFiles;
  char    title[11];
  WORD    DScrc;
  char    DSsignature[6+3];
  BYTE    DSfileMap[0x80];
  BYTE    DSfolderMap[0x80];
  char    DSfolders[0x7F][11];
  BYTE    DSend;
  BYTE    empty[0x7F];
};

struct HoHdr
{
  char name[8];
  char type;
  WORD start;
  WORD length;
  BYTE reserved;
  BYTE noSecs;
  WORD checkSum;
};

struct SCLFileHdr
{
  char name[8];
  char type;
  WORD start;
  WORD length;
  BYTE noSecs;
};

#endif
