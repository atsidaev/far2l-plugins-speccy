#ifndef td_hpp
#define td_hpp
#include <windows.h>
#include "teledisk.hpp"

void td_init       (HANDLE hostFile, const char* fileName, ImageHdr imgHdr_, bool isPacked_);
bool read_         (BYTE* buf, WORD size);
bool unRLE         (BYTE* packedSec, short secSize, BYTE* sec);
WORD calculateCRC16(BYTE* buf, WORD size);

#endif
