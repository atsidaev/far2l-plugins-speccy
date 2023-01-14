#include <windows.h>

bool WINAPI _export fdi_isImage(char* fileName, const BYTE* data, int size);
HANDLE WINAPI _export fdi_openSubPlugin(char* fileName);
bool WINAPI _export fdi_closeSubPlugin(HANDLE h);
bool WINAPI _export fdi_reload (HANDLE h);
bool WINAPI _export fdi_openFile(HANDLE h);
bool WINAPI _export fdi_closeFile(HANDLE h);
bool WINAPI _export fdi_read(HANDLE h, WORD blockNum, BYTE* buf);
bool WINAPI _export fdi_write(HANDLE h, WORD blockNum, BYTE* buf);
char* WINAPI _export fdi_getFormatName (void);