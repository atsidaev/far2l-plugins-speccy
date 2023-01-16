#ifndef tools_hpp
#define tools_hpp
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "types.hpp"

bool  compareMemoryIgnoreCase(u8* p1, u8* p2, u16 size);
const char* pointToName    (const char *path);
const char* pointToExt     (const char *path);
void  addEndSlash    (char *path);
char* getMsg         (int msgId);
void  initDialogItems(InitDialogItem *init, FarDialogItem *item, int noItems);
int   messageBox     (u32 flags, char **items, int noItems, int noButtons);
bool  compareMemory  (const void* p1, const void* p2, u16 size);
char* cropLongName   (char* newName, const char* name, int noChars);
bool  isValidChar    (char ch);
char* makeFullName   (char* fullName, const char* path, const char* name);

void  makeCompatibleFileName  (char* newName, const char* oldName);
void  makeCompatibleFolderName(char* newName, const char* oldName);

#endif
