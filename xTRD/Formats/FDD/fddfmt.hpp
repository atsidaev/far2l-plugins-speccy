#include <windows.h>

bool   fdd_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE fdd_init         (const char* fileName);
bool   fdd_cleanup      (HANDLE h);
bool   fdd_reload       (HANDLE h);
bool   fdd_open         (HANDLE h);
bool   fdd_close        (HANDLE h);
bool   fdd_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   fdd_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  fdd_getFormatName(void);
bool   fdd_isProtected  (HANDLE h);
bool   fdd_protect      (HANDLE h, bool on);
