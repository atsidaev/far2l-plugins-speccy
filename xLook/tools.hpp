#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "far2sdk/farplug-mb.h"
using namespace oldfar;

void errorMessage(char *msg);
int messageBox(unsigned long flags, char **items, int noItems, int noButtons);
char* point2name(char *path);
WORD calculateCheckSum(BYTE* p);
bool isEnAlphaNum(BYTE ch);
bool isValidChar(BYTE ch);
void makeTrDosName(char* dest, const BYTE* hdr);
void makePCName(char *from, char *to);
char* truncPathStr(char *newName, const char *name, int noChars);
int memcmpi(const char* p1, const char* p2, int maxlen);
FarDialogItem InitItem(int type, int x1, int y1, int x2, int y2, int focus, const void* param, DWORD flags, int def, const char* data);

#endif