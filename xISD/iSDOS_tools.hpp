#ifndef iSDOS_tools_hpp
#define iSDOS_tools_hpp
#include "iSDOS.hpp"
#include <windows.h>

bool       isDir      (const UniHdr& h);
u32        getSize    (const UniHdr& h);
void       setFileSize(UniHdr& h, int size);
void       makeName   (const UniHdr& h, u8* buf);
SYSTEMTIME makeDate   (const UniHdr& h);
void       setDate    (UniHdr& h, const SYSTEMTIME& t);
u16        findFile   (const UniHdr* pDir, const char* fileName);
#endif
