#ifndef FMT_HPP
#define FMT_HPP

#include <windows.h>

extern "C"
{
  bool   WINAPI _export isImage      (const char* fileName, const BYTE* data, int size);
  HANDLE WINAPI _export init         (const char* fileName);
  void   WINAPI _export cleanup      (HANDLE h);
  bool   WINAPI _export reload       (HANDLE h);
  bool   WINAPI _export open         (HANDLE h);
  bool   WINAPI _export close        (HANDLE h);
  bool   WINAPI _export read         (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  bool   WINAPI _export write        (HANDLE h, BYTE trk, BYTE sec, BYTE* buf);
  char*  WINAPI _export getFormatName(void);
  bool   WINAPI _export isProtected  (HANDLE h);
  bool   WINAPI _export protect      (HANDLE h, bool on);
}

#endif
