#include <windows.h>

bool   udi_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE udi_init         (const char* fileName);
bool   udi_cleanup      (HANDLE h);
bool   udi_reload       (HANDLE h);
bool   udi_open         (HANDLE h);
bool   udi_close        (HANDLE h);
bool   udi_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   udi_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  udi_getFormatName(void);
bool   udi_isProtected  (HANDLE h);
bool   udi_protect      (HANDLE h, bool on);
