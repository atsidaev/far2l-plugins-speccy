#include <windows.h>
#include "types.hpp"

BYTE *curTo, *curFrom;
WORD toSize_, fromSize_;
WORD noBytesRead, noBytesWritten;

WORD getByte(BYTE& b)
{
  if(noBytesRead == fromSize_) return false;
  b = *curFrom++;
  ++noBytesRead;
  return true;
}

bool putByte(BYTE b)
{
  if(noBytesWritten == toSize_) return false;
  *curTo++ = b;
  ++noBytesWritten;
  return true;
}
