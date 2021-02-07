#ifndef DETECTOR_HPP
#define DETECTOR_HPP

#include <windows.h>
#include "types.hpp"

struct FormatInfo
{
  BYTE    type;
  int     size;
  int     start;
  int     noSecs;

  BYTE    signature[32];
  WORD    signatureOffset;
  DWORD   signatureMask;
  BYTE    signatureSize;

  WORD    commentOffset;
  BYTE    commentSize;
  
  BYTE    specialChar;
  BYTE    newType;
  bool    skipHeader;
  char*   description;
};

class Detector
{
  public:
    Detector(char* path);
    ~Detector();

    // возвращает 0xFF если тип не определен
    BYTE  detect       (const FileHdr& hdr, const BYTE* secs, int size, char* comment);
    
    void  specialChar  (BYTE n, char *pos);
    void  getType      (BYTE n, char *pos);
    bool  getSkipHeader(BYTE n);
    // возвращает 0, если
    // соответствующий параметр не задан
    char* description  (BYTE n);
  private:
    BYTE       noFormats;
    FormatInfo formats[255];
};

#endif
