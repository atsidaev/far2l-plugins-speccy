#ifndef FMTREADER_HPP
#define FMTREADER_HPP
#include <windows.h>
#include "far2sdk/farplug-mb.h"

typedef bool   (WINAPI *IS_IMAGE)(const char *name, const unsigned char* data, int size);
typedef HANDLE (WINAPI *INIT)    (const char* fileName);
typedef bool   (WINAPI *CLEANUP) (HANDLE);
typedef bool   (WINAPI *RELOAD)  (HANDLE);
typedef bool   (WINAPI *OPEN)    (HANDLE);
typedef bool   (WINAPI *CLOSE)   (HANDLE);
typedef bool   (WINAPI *READ)    (HANDLE h, unsigned char trk, unsigned char sec, unsigned char* buf);
typedef bool   (WINAPI *WRITE)   (HANDLE h, unsigned char trk, unsigned char sec, unsigned char* buf);

typedef char*  (WINAPI *GET_FORMAT_NAME)(void);
typedef bool   (WINAPI *IS_PROTECTED)   (HANDLE);
typedef bool   (WINAPI *PROTECT)        (HANDLE h, bool);

struct FmtPlugin
{
  HMODULE  handle;
  
  IS_IMAGE isImage;
  INIT     init;
  CLEANUP  cleanup;
  RELOAD   reload;
  OPEN     open;
  CLOSE    close;
  READ     read;
  WRITE    write;
  
  GET_FORMAT_NAME getFormatName;
  IS_PROTECTED    isProtected;
  PROTECT         protect;
};

class FmtReader
{
 public:
   FmtReader         ();
   ~FmtReader        ();
   FmtPlugin *isImage(char *fileName, const BYTE *data, int size);
 private:
//   FmtReader         (char *path);
   int noPlugins = 0;
   FmtPlugin plugins[16];
};

#endif
