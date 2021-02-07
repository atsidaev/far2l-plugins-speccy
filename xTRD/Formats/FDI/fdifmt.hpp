#include <windows.h>

bool   WINAPI _export fdi_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE WINAPI _export fdi_init         (const char* fileName);
bool   WINAPI _export fdi_cleanup      (HANDLE h);
bool   WINAPI _export fdi_reload       (HANDLE h);
bool   WINAPI _export fdi_open         (HANDLE h);
bool   WINAPI _export fdi_close        (HANDLE h);
bool   WINAPI _export fdi_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   WINAPI _export fdi_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  WINAPI _export fdi_getFormatName(void);
bool   WINAPI _export fdi_isProtected  (HANDLE h);
bool   WINAPI _export fdi_protect      (HANDLE h, bool on);

