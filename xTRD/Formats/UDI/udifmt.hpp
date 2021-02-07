#include <windows.h>

bool   WINAPI _export udi_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE WINAPI _export udi_init         (const char* fileName);
bool   WINAPI _export udi_cleanup      (HANDLE h);
bool   WINAPI _export udi_reload       (HANDLE h);
bool   WINAPI _export udi_open         (HANDLE h);
bool   WINAPI _export udi_close        (HANDLE h);
bool   WINAPI _export udi_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   WINAPI _export udi_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  WINAPI _export udi_getFormatName(void);
bool   WINAPI _export udi_isProtected  (HANDLE h);
bool   WINAPI _export udi_protect      (HANDLE h, bool on);
