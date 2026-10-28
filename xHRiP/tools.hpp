#pragma once
#include "types.hpp"
#include "far2sdk/farplug-mb.h"
using namespace oldfar;
void errorMessage(char * message);
void makeTrDosName(char * dest, const FileHdr & hdr, int width);
char * getMsg(int id);
void initDialogItems(InitDialogItem * init, FarDialogItem * items, int count);
WORD fileSize(const FileHdr & hdr);
