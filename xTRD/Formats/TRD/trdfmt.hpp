#ifndef trd_HPP
#define trd_HPP

#include <windows.h>

  bool   WINAPI _export trd_isImage      (const char* fileName, const BYTE* data, int size);
  HANDLE WINAPI _export trd_init         (const char* fileName);
  bool   WINAPI _export trd_cleanup      (HANDLE h);
  bool   WINAPI _export trd_reload       (HANDLE h);
  bool   WINAPI _export trd_open         (HANDLE h);
  bool   WINAPI _export trd_close        (HANDLE h);
  bool   WINAPI _export trd_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  bool   WINAPI _export trd_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  char*  WINAPI _export trd_getFormatName(void);
  bool   WINAPI _export trd_isProtected  (HANDLE h);
  bool   WINAPI _export trd_protect      (HANDLE h, bool on);

#endif
