#include <windows.h>

bool   fdi_isImage      (const char* fileName, const BYTE* data, int size);
HANDLE fdi_init         (const char* fileName);
bool   fdi_cleanup      (HANDLE h);
bool   fdi_reload       (HANDLE h);
bool   fdi_open         (HANDLE h);
bool   fdi_close        (HANDLE h);
bool   fdi_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
bool   fdi_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
char*  fdi_getFormatName(void);
bool   fdi_isProtected  (HANDLE h);
bool   fdi_protect      (HANDLE h, bool on);

