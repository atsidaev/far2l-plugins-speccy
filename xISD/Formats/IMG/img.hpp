#include <windows.h>

bool WINAPI _export img_isImage(char* fileName, const BYTE* data, int size);
HANDLE WINAPI _export img_openSubPlugin(char* fileName);
bool WINAPI _export img_closeSubPlugin(HANDLE h);
bool WINAPI _export img_reload (HANDLE h);
bool WINAPI _export img_openFile(HANDLE h);
bool WINAPI _export img_closeFile(HANDLE h);
bool WINAPI _export img_read(HANDLE h, WORD blockNum, BYTE* buf);
bool WINAPI _export img_write(HANDLE h, WORD blockNum, BYTE* buf);
char* WINAPI _export img_getFormatName (void);