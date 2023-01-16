#include <windows.h>

bool   td_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE td_init         (const char* fileName);
bool   td_cleanup      (HANDLE h);
bool   td_reload       (HANDLE h);
bool   td_open         (HANDLE h);
bool   td_close        (HANDLE h);
bool   td_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   td_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  td_getFormatName(void);
bool   td_isProtected  (HANDLE h);
bool   td_protect      (HANDLE h, bool on);

