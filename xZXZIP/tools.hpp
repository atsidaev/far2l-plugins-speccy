#pragma once
#include "types.hpp"
#include "far2sdk/farplug-mb.h"
using namespace oldfar;
void errorMessage(char * message);
void makeTrDosName(char * dest, const ZipedFileHdr & hdr, int width);
char * getMsg(int id);
void initDialogItems(InitDialogItem * init, FarDialogItem * items, int count);
WORD fileSize(const ZipedFileHdr & hdr);
bool isValidZippedFile(ZipedFileHdr & hdr);
DWORD crc32(BYTE * data, int size);
