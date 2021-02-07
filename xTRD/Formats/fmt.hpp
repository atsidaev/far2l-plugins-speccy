#ifndef FMT_HPP
#define FMT_HPP

#include <windows.h>

extern "C"
{
  bool   WINAPI _export fmt_isImage      (const char* fileName, const BYTE* data, int size);
  HANDLE WINAPI _export fmt_init         (const char* fileName);
  bool   WINAPI _export fmt_cleanup      (HANDLE h);
  bool   WINAPI _export fmt_reload       (HANDLE h);
  bool   WINAPI _export fmt_open         (HANDLE h);
  bool   WINAPI _export fmt_close        (HANDLE h);
  bool   WINAPI _export fmt_read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  bool   WINAPI _export fmt_write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  char*  WINAPI _export fmt_getFormatName(void);
  bool   WINAPI _export fmt_isProtected  (HANDLE h);
  bool   WINAPI _export fmt_protect      (HANDLE h, bool on);
}

#endif
