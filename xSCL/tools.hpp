#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <windows.h>
#include "types.hpp"
#include "plugin.hpp"

bool  compareMemory (BYTE* p1, BYTE* p2, WORD size);
bool  compareMemoryIgnoreCase(BYTE* p1, BYTE* p2, WORD size);
char* pointToName   (char *path);
char* pointToExt    (char *path);
bool  isValidChar   (BYTE ch);
bool  isAlphaNumeric(BYTE ch);
void  addEndSlash   (char *path);
char* trim          (char *str);

char* getMsg(int msgId);
void  initDialogItems(InitDialogItem *init, FarDialogItem *item, int noItems);

DWORD calculateCheckSum(BYTE* ptr, WORD size);
WORD  calculateCheckSum(HoHdr& hdr);

void errorMessage(char *msg);

DWORD copySectors(HANDLE to, HANDLE from, int noSecs);

bool isSCLFull(int no_files, int no_add_files, HANDLE file);

void makeTrDosName(char* dest, const FileHdr& hdr, int width);
int  messageBox   (DWORD flags, char **items, int noItems, int noButtons);

FileType detectFileType(HANDLE file);
#endif
