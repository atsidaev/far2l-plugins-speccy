#ifndef __PT2__
#define __PT2__

#include "module.hpp"

#pragma pack(push, 1)
struct pt3Header
{
  char name[99];
  BYTE table;
  BYTE delay;
  BYTE noPositions;
  BYTE loopPosition;
  WORD patternsPointer;
  WORD samplePointers[32];
  WORD ornamentsPointers[16];
};
#pragma pack(pop)

int is_pt3(const BYTE *data, int size);
bool quickView_pt3(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize);

#endif
