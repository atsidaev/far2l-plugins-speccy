#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "types.hpp"

bool compareMemory(BYTE* p1, BYTE* p2, WORD size);
bool compareMemoryIgnoreCase(BYTE* p1, BYTE* p2, WORD size);
char* pointToName   (char *path);
char* pointToExt    (char *path);

void errorMessage(char *msg);
int  messageBox  (unsigned long flags, char **items, int noItems, int noButtons);

char* pointToName   (char *path);
char* trim          (char *str);
char* truncPathStr  (char *newName, const char *name, int noChars);
void  addEndSlash   (char *path);
void  quoteSpaceOnly(char *buf);

bool isValidChar      (BYTE ch);
bool isValidFolderChar(BYTE ch); // ࠧ�蠥� ���᪨� �㪢�

char *str_r_chr(const char *s, int c);
// int memcmp (BYTE* p1, BYTE* p2, int maxlen);
// �ࠢ����� �����⥩ ����� ��� ��� ॣ����
int memcmpi(const char* p1, const char* p2, int maxlen);
int strcmpi(const char* p1, const char* p2);

DWORD copySectors(HANDLE to, HANDLE from, int noSecs);

bool isSCLFull(int no_files, int no_add_files, HANDLE file);

bool isEnAlphaNum (BYTE ch);
void makeTrDosName(char* dest, const FileHdr& hdr, int width);

WORD crc          (BYTE* ptr, WORD len);

DWORD calculateCheckSum(BYTE* ptr, WORD size);
WORD  calculateCheckSum(HoHdr& hdr);

char* make8x3name(const char* source, char* dest);

char* getMsg         (int msgId);
void  initDialogItems(InitDialogItem *init, FarDialogItem *item, int noItems);

FileType detectFileType(HANDLE file);
#endif
