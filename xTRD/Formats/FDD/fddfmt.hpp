#include <windows.h>

bool   WINAPI _export fdd_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE WINAPI _export fdd_init         (const char* fileName);
bool   WINAPI _export fdd_cleanup      (HANDLE h);
bool   WINAPI _export fdd_reload       (HANDLE h);
bool   WINAPI _export fdd_open         (HANDLE h);
bool   WINAPI _export fdd_close        (HANDLE h);
bool   WINAPI _export fdd_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   WINAPI _export fdd_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  WINAPI _export fdd_getFormatName(void);
bool   WINAPI _export fdd_isProtected  (HANDLE h);
bool   WINAPI _export fdd_protect      (HANDLE h, bool on);
