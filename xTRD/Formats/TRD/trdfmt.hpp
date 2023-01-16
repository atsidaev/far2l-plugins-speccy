#ifndef trd_HPP
#define trd_HPP

#include <windows.h>

  bool   trd_isImage      (const char* fileName, const BYTE* data, int size);
  HANDLE trd_init         (const char* fileName);
  bool   trd_cleanup      (HANDLE h);
  bool   trd_reload       (HANDLE h);
  bool   trd_open         (HANDLE h);
  bool   trd_close        (HANDLE h);
  bool   trd_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  bool   trd_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  char*  trd_getFormatName(void);
  bool   trd_isProtected  (HANDLE h);
  bool   trd_protect      (HANDLE h, bool on);

#endif
