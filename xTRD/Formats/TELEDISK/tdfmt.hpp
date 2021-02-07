#include <windows.h>

bool   WINAPI _export td_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE WINAPI _export td_init         (const char* fileName);
bool   WINAPI _export td_cleanup      (HANDLE h);
bool   WINAPI _export td_reload       (HANDLE h);
bool   WINAPI _export td_open         (HANDLE h);
bool   WINAPI _export td_close        (HANDLE h);
bool   WINAPI _export td_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   WINAPI _export td_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  WINAPI _export td_getFormatName(void);
bool   WINAPI _export td_isProtected  (HANDLE h);
bool   WINAPI _export td_protect      (HANDLE h, bool on);

