#include <windows.h>
#include "../fmt.hpp"
#include "../../../shared/widestring.hpp"

const unsigned short sectorSize = 0x100;

struct ImageInfo
{
  char   fileName[300];
  HANDLE hostFile;
};

bool WINAPI _export trd_isImage(const char* fileName, const BYTE* data, int size)
{
  if(size < 0x8E8) return false;
  if(data[0x8E7] != 0x10) return false;

  WIN32_FIND_DATA findData;
  HANDLE hostFile = FindFirstFile(_W((char*)fileName).c_str(), &findData);
  if(hostFile == INVALID_HANDLE_VALUE) return false;
  FindClose(hostFile);

  if(findData.nFileSize%sectorSize || findData.nFileSize < 16*sectorSize)
    return false;
  return true;
}

HANDLE WINAPI _export trd_init(const char* fileName)
{
  ImageInfo* ii = new ImageInfo;
  strcpy(ii->fileName, fileName);
  return (HANDLE)ii;
}

bool WINAPI _export trd_cleanup(HANDLE h)
{
  ImageInfo* ii = (ImageInfo*)h;
  delete ii;
  return 1;
}

bool WINAPI _export trd_reload(HANDLE h) { return true; }

bool WINAPI _export trd_open(HANDLE h)
{
  ImageInfo* ii = (ImageInfo*)h;

  DWORD mode = GENERIC_READ | GENERIC_WRITE;

  DWORD attr = GetFileAttributes(_W(ii->fileName).c_str());

  if(attr & FILE_ATTRIBUTE_READONLY) mode = GENERIC_READ;
  
  ii->hostFile = CreateFile(_W(ii->fileName).c_str(),
                            mode,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
  return (ii->hostFile != INVALID_HANDLE_VALUE);
}

bool WINAPI _export trd_close(HANDLE h)
{
  ImageInfo* ii = (ImageInfo*)h;
  return CloseHandle(ii->hostFile);
}

bool WINAPI _export trd_read(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  ImageInfo* ii = (ImageInfo*)h;  

  memset(buf, 0, sectorSize);
  DWORD fileSize = GetFileSize(ii->hostFile, NULL);
  if(fileSize <= sectorSize*(16U*trk+sec)) return false;
  
  SetFilePointer(ii->hostFile, sectorSize*(16*trk+sec), NULL, FILE_BEGIN);
  
  DWORD noBytesRead;
  ReadFile(ii->hostFile, buf, sectorSize, &noBytesRead, NULL);
  
  return (noBytesRead == sectorSize);
}

bool WINAPI _export trd_write(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  ImageInfo* ii = (ImageInfo*)h;
  
  DWORD fileSize = GetFileSize(ii->hostFile, NULL);
  if(fileSize < sectorSize*(16U*trk+sec)) return false;
  
  SetFilePointer(ii->hostFile, sectorSize*(16*trk+sec), NULL, FILE_BEGIN);
  
  DWORD noBytesWritten;
  WriteFile(ii->hostFile, buf, sectorSize, &noBytesWritten, NULL);
  
  return (noBytesWritten == sectorSize);
}

char* WINAPI _export trd_getFormatName(void)
{
  static char* name = "TRD";
  return name;
}

bool WINAPI _export trd_isProtected(HANDLE h)
{
  ImageInfo* ii   = (ImageInfo*)h;
  DWORD      attr = GetFileAttributes(_W(ii->fileName).c_str());
  return (attr & FILE_ATTRIBUTE_READONLY);
}

bool WINAPI _export trd_protect(HANDLE h, bool on)
{
  ImageInfo* ii   = (ImageInfo*)h;
  DWORD      attr = GetFileAttributes(_W(ii->fileName).c_str());
  if(on)
    attr |= FILE_ATTRIBUTE_READONLY;
  else
    attr &= ~FILE_ATTRIBUTE_READONLY;

  return (SetFileAttributes(_W(ii->fileName).c_str(), attr));
}
