#ifndef FmtPlugin_hpp
#define FmtPlugin_hpp
#include <windows.h>

typedef bool   (WINAPI *IS_IMAGE)         (char *name, const unsigned char* data, int size);
typedef HANDLE (WINAPI *OPEN_SUB_PLUGIN)  (char* fileName);
typedef bool   (WINAPI *CLOSE_SUB_PLUGIN) (HANDLE);
typedef bool   (WINAPI *RELOAD)           (HANDLE);
typedef bool   (WINAPI *OPEN_FILE)        (HANDLE);
typedef bool   (WINAPI *CLOSE_FILE)       (HANDLE);
typedef bool   (WINAPI *READ)             (HANDLE h, unsigned short block, unsigned char* buf);
typedef bool   (WINAPI *WRITE)            (HANDLE h, unsigned short block, unsigned char* buf);
typedef char*  (WINAPI *GET_FORMAT_NAME)  (void);

struct FmtPlugin
{
  HINSTANCE handle;
  
  IS_IMAGE         isImage;
  OPEN_SUB_PLUGIN  openSubPlugin;
  CLOSE_SUB_PLUGIN closeSubPlugin;
  RELOAD           reload;
  OPEN_FILE        openFile;
  CLOSE_FILE       closeFile;
  READ             read;
  WRITE            write;
  GET_FORMAT_NAME  getFormatName;
};

#endif
