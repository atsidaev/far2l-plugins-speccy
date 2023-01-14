#include <windows.h>
#include "..\fmt.hpp"

const unsigned short blockSize = 0x100;

struct Filer
{
  HANDLE hostFile;
  char   fName[300];
};

bool WINAPI _export isImage(char* fileName, const BYTE* data, int size)
{  
  if((data[10] != 'D' || data[11] != 'S' || data[12] != 'K') &&
     (data[13] != 'D' || data[14] != 'S' || data[15] != 'K'))
    return false;

  WIN32_FIND_DATA findData;
  HANDLE hostFile = FindFirstFile(fileName, &findData);
  if(hostFile == INVALID_HANDLE_VALUE) return false;
  FindClose(hostFile);

  if(findData.nFileSizeLow < 3*blockSize || findData.nFileSizeLow%blockSize)
    return false;
  return true;
}

HANDLE WINAPI _export openSubPlugin(char* fileName)
{
  Filer* f = (Filer*)malloc(sizeof(Filer));
  lstrcpy(f->fName, fileName);
  return (HANDLE)f;
  
}

void WINAPI _export closeSubPlugin(HANDLE h)
{
  free(h);
}

bool WINAPI _export reload (HANDLE h) { return true; }

bool WINAPI _export openFile(HANDLE h)
{
  Filer* f = (Filer*)h;
  DWORD mode = GENERIC_READ | GENERIC_WRITE;
  DWORD attr = GetFileAttributes(f->fName);
  if(attr & FILE_ATTRIBUTE_READONLY) mode = GENERIC_READ;
  f->hostFile = CreateFile(f->fName,
                           mode,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL);
  return (f->hostFile != INVALID_HANDLE_VALUE);
}

bool WINAPI _export closeFile(HANDLE h)
{
  Filer* f = (Filer*)h;
  return CloseHandle(f->hostFile);
}

bool WINAPI _export read(HANDLE h, WORD blockNum, BYTE* buf)
{
  ZeroMemory(buf, blockSize);
  Filer* f = (Filer*)h;
  SetFilePointer(f->hostFile, blockSize*blockNum, NULL, FILE_BEGIN);
  DWORD noBytesRead;
  ReadFile(f->hostFile, buf, blockSize, &noBytesRead, NULL);
  return (noBytesRead == blockSize);
}

bool WINAPI _export write(HANDLE h, WORD blockNum, BYTE* buf)
{
  Filer* f = (Filer*)h;
  SetFilePointer(f->hostFile, blockSize*blockNum, NULL, FILE_BEGIN);
  DWORD noBytesWritten;
  WriteFile(f->hostFile, buf, blockSize, &noBytesWritten, NULL);
  return (noBytesWritten == blockSize);
}

char* WINAPI _export getFormatName (void)
{
  static char* name = "IMG";
  return name;
}
