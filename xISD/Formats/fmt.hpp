#ifndef fmt_hpp
#define fmt_hpp

#include <windows.h>

extern "C"
{
  bool   WINAPI _export isImage       (char* fileName, const BYTE* data, int size);
  HANDLE WINAPI _export openSubPlugin (char* fileName);
  void   WINAPI _export closeSubPlugin(HANDLE h);
  bool   WINAPI _export reload        (HANDLE h);
  bool   WINAPI _export openFile      (HANDLE h);
  bool   WINAPI _export closeFile     (HANDLE h);
  bool   WINAPI _export read          (HANDLE h, WORD blockNum, BYTE* buf);
  bool   WINAPI _export write         (HANDLE h, WORD blockNum, BYTE* buf);
  char*  WINAPI _export getFormatName (void);
}

#endif