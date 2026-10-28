#ifndef __PT3__
#define __PT3__

#include "module.hpp"

#pragma pack(push, 1)
struct pt2Header
{
  BYTE delay;
  BYTE noPositions;
  BYTE loopPosition;
  WORD samplePointers[32];
  WORD ornamentPointers[16];
  WORD patternsPointer;
  char name[30];
};
#pragma pack(pop)

int is_pt2(const BYTE *data, int size);
bool quickView_pt2(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize);

#endif
