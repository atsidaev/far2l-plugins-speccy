#include <windows.h>
#include "..\fmt.hpp"

const unsigned short sectorSize = 0x100;

struct ImageInfo
{
  char   fileName[300];
  HANDLE hostFile;
};

bool WINAPI _export isImage(const char* fileName, const BYTE* data, int size)
{
  if(size < 0x8E8) return false;
  if(data[0x8E7] != 0x10) return false;

  WIN32_FIND_DATA findData;
  HANDLE hostFile = FindFirstFile(fileName, &findData);
  if(hostFile == INVALID_HANDLE_VALUE) return false;
  FindClose(hostFile);

  if(findData.nFileSizeLow%sectorSize || findData.nFileSizeLow < 16*sectorSize)
    return false;
  return true;
}

HANDLE WINAPI _export init(const char* fileName)
{
  ImageInfo* ii = new ImageInfo;
  lstrcpy(ii->fileName, fileName);
  return (HANDLE)ii;
}

void WINAPI _export cleanup(HANDLE h)
{
  ImageInfo* ii = (ImageInfo*)h;
  delete ii;
}

bool WINAPI _export reload(HANDLE h) { return true; }

bool WINAPI _export open(HANDLE h)
{
  ImageInfo* ii = (ImageInfo*)h;

  DWORD mode = GENERIC_READ | GENERIC_WRITE;

  DWORD attr = GetFileAttributes(ii->fileName);

  if(attr & FILE_ATTRIBUTE_READONLY) mode = GENERIC_READ;
  
  ii->hostFile = CreateFile(ii->fileName,
                            mode,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
  return (ii->hostFile != INVALID_HANDLE_VALUE);
}

bool WINAPI _export close(HANDLE h)
{
  ImageInfo* ii = (ImageInfo*)h;
  return CloseHandle(ii->hostFile);
}

bool WINAPI _export read(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  ImageInfo* ii = (ImageInfo*)h;  

  ZeroMemory(buf, sectorSize);
  DWORD fileSize = GetFileSize(ii->hostFile, NULL);
  if(fileSize <= sectorSize*(16U*trk+sec)) return false;
  
  SetFilePointer(ii->hostFile, sectorSize*(16*trk+sec), NULL, FILE_BEGIN);
  
  DWORD noBytesRead;
  ReadFile(ii->hostFile, buf, sectorSize, &noBytesRead, NULL);
  
  return (noBytesRead == sectorSize);
}

bool WINAPI _export write(HANDLE h, BYTE trk, BYTE sec, BYTE* buf)
{
  ImageInfo* ii = (ImageInfo*)h;
  
  DWORD fileSize = GetFileSize(ii->hostFile, NULL);
  if(fileSize < sectorSize*(16U*trk+sec)) return false;
  
  SetFilePointer(ii->hostFile, sectorSize*(16*trk+sec), NULL, FILE_BEGIN);
  
  DWORD noBytesWritten;
  WriteFile(ii->hostFile, buf, sectorSize, &noBytesWritten, NULL);
  
  return (noBytesWritten == sectorSize);
}

char* WINAPI _export getFormatName(void)
{
  static char* name = "TRD";
  return name;
}

bool WINAPI _export isProtected(HANDLE h)
{
  ImageInfo* ii   = (ImageInfo*)h;
  DWORD      attr = GetFileAttributes(ii->fileName);
  return (attr & FILE_ATTRIBUTE_READONLY);
}

bool WINAPI _export protect(HANDLE h, bool on)
{
  ImageInfo* ii   = (ImageInfo*)h;
  DWORD      attr = GetFileAttributes(ii->fileName);
  if(on)
    attr |= FILE_ATTRIBUTE_READONLY;
  else
    attr &= ~FILE_ATTRIBUTE_READONLY;

  return (SetFileAttributes(ii->fileName, attr));
}
