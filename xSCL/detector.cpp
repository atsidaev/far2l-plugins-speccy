#include <windows.h>
#include "plugin.hpp"

#include "detector.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

HANDLE ini;
char   iniBuf[256];
DWORD  bufSize;
DWORD  index;

bool fgetch(char& ch)
{
  if(index == bufSize)
  {
    ReadFile(ini, iniBuf, sizeof(iniBuf), &bufSize, NULL);
    if(bufSize == 0) return false;
    index = 0;
  }
  ch = iniBuf[index++];
  return true;
}

bool fgets(char* to, int maxSize)
{
  static bool eof = false;
  if(eof) return false;

  int i = 0;
  for(; i < maxSize - 1; ++i)
  {
    char ch;
    if(!fgetch(ch))
    {
      eof = true;
      break;
    }
    if(ch == 0x0a) break;
    if(ch != 0x0d)
      to[i] = ch;
    else
      --i;
  }
  to[i] = 0;
  return true;
}

char* skipWS(char* ptr)
{
  while(*ptr && (*ptr == ' ' || *ptr == '\t')) ++ptr;
  return ptr;
}

bool parseLine(char* ptr, char* key, char* value)
{
  while(*ptr && !(*ptr == ' ' || *ptr == '\t' || *ptr == '=')) *key++ = *ptr++;
  *key = 0;
  ptr = skipWS(ptr);
  if(!*ptr || *ptr != '=') return false;
  ptr = skipWS(++ptr);
  if(!*ptr) return false;

  char* end = ptr + lstrlen(ptr) - 1;
  while(end != ptr && (*end == ' ' || *end == '\t')) --end; // skip end ws
  while(ptr != end+1) *value++ = *ptr++;
  *value = 0;
  return true;
}

int atoi(char* ptr)
{
  int  result = -1;
  int  factor = 10;

  if(*ptr == '#') { factor = 0x10; ++ptr;}
  if(*ptr == '0' && *(ptr+1) == 'x') { factor = 0x10; ptr += 2; }
  
  while(1)
  {
    BYTE digit = 0xFF;
    if(*ptr >= '0' && *ptr <= '9') digit = *ptr - '0';
    if(factor == 0x10)
    {
      if(*ptr >= 'A' && *ptr <= 'F') digit = *ptr - 'A' + 10;
      if(*ptr >= 'a' && *ptr <= 'f') digit = *ptr - 'a' + 10;
    }
    if(digit == 0xFF) break;
    if(result == -1) result = 0;
    
    result = factor*result + digit;
    ++ptr;
  }
  return result;
}

bool getByte(BYTE& b, char* ptr)
{
  if(*ptr == '\'' && *(ptr+2) == '\'')
  {
    b = *(ptr+1);
    return true;
  }
  int num = atoi(ptr);
  if(num < 0 || num > 255) return false;
  b = num;
  return true;
}

bool getWord(WORD& w, char* ptr)
{
  if(*ptr == '\'' && *(ptr+3) == '\'')
  {
    w = 256*(*(ptr+2)) + *(ptr+1);
    return true;
  }
  int num = atoi(ptr);
  if(num < 0 || num > 65535) return false;
  w = num;
  return true;
}

bool checkFormat(const FormatInfo& form)
{
  if(form.type          ==  0  &&
     form.size          == -1 &&
     form.start         == -1 &&
     form.noSecs        == -1 &&
     form.signatureSize ==  0) return false;
  
  if(form.specialChar   == 0 &&
     form.newType       == 0 &&
     form.commentSize   == 0 &&
     form.description   == 0) return false;
  return true;
}

Detector::Detector(char* path)
{
  char fileName[300];
  noFormats = 0;
  while(1)
  {
    if(op.iniFilePath[0] != 0)
    {
      lstrcpy(fileName, op.iniFilePath);
      ini = CreateFile(fileName,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);

      if(ini != INVALID_HANDLE_VALUE) break;
    }

    lstrcpy(fileName, path);
    lstrcpy(pointToName(fileName), "types.ini");

    ini = CreateFile(fileName,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_FLAG_SEQUENTIAL_SCAN,
                     NULL);

    if(ini != INVALID_HANDLE_VALUE) break;

    lstrcpy(fileName, path);
    char *ptr = pointToName(fileName) - 1;
    *ptr = 0;
    lstrcpy(pointToName(fileName), "types.ini");

    ini = CreateFile(fileName,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_FLAG_SEQUENTIAL_SCAN,
                     NULL);

    if(ini != INVALID_HANDLE_VALUE) break;

    char *msgItems[3];
    msgItems[0] = getMsg(MxSCLWarning);
    msgItems[1] = getMsg(MCanNotOpenINI);
    msgItems[2] = getMsg(MOk);
    messageBox(FMSG_WARNING | FMSG_DOWN, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);

    return;
  }

  index = bufSize = sizeof(iniBuf);
  
  char  line [256];
  char  key  [20];
  char  value[128];
  char* ptr;
  
  bool errors = false;

  FormatInfo form;
  ZeroMemory(&form, sizeof(FormatInfo));
  form.size   = -1;
  form.start  = -1;
  form.noSecs = -1;
  
  while(fgets(line, sizeof(line)))
  {
    if(noFormats == 255) break;
    ptr = skipWS(line);
    
    // пропускаем пустые строки и коментарии
    if(!*ptr || *ptr == '#' || *ptr == ';') continue;

    if(*ptr == '[')
    {
      if(checkFormat(form)) formats[noFormats++] = form;
      
      ZeroMemory(&form, sizeof(FormatInfo));
      form.size   = -1;
      form.start  = -1;
      form.noSecs = -1;
      continue;
    }

    if(!parseLine(ptr, key, value))
    {
      errors = true;
      continue;
    }

    if(!lstrcmp(key, "Type"))
    {
      BYTE type;
      if(getByte(type, value))
        form.type = type;
      else
        errors = true;
      continue;
    }

    if(!lstrcmp(key, "Size"))
    {
      WORD size;
      if(getWord(size, value))
        form.size = size;
      else
        errors = true;
      continue;
    }

    if(!lstrcmp(key, "Start"))
    {
      WORD start;
      if(getWord(start, value))
        form.start = start;
      else
        errors = true;
      continue;
    }

    if(!lstrcmp(key, "NoSecs"))
    {
      BYTE noSecs;
      if(getByte(noSecs, value))
        form.noSecs = noSecs;
      else
        errors = true;
      continue;
    }

    if(!lstrcmp(key, "NewType"))
    {
      BYTE newType;
      if(getByte(newType, value))
        form.newType = newType;
      else
        errors = true;
      continue;
    }

    if(!lstrcmp(key, "SpecialChar"))
    {
      BYTE specialChar;
      if(getByte(specialChar, value))
        form.specialChar = specialChar;
      else
        errors = true;
      continue;
    }

    if(!lstrcmp(key, "Description"))
    {
      if(form.description) delete[] form.description;
      form.description = new char[lstrlen(value)+1];
      lstrcpy(form.description, value);
    }

    if(!lstrcmp(key, "ShowHeader"))
    {
      if(!lstrcmpi(value, "no")) form.skipHeader = true;
    }

    if(!lstrcmp(key, "Comment"))
    {
      WORD w;
      ptr = value;
      if(getWord(w, ptr))
        form.commentOffset = w;
      else
      {
        errors = true;
        continue;
      }
      while(*ptr && !(*ptr == ' ' || *ptr == '\t')) ++ptr;
      ptr = skipWS(ptr);

      BYTE b;
      if(getByte(b, ptr))
        form.commentSize = b;
      else
        errors = true;
      continue;
    }

    if(!lstrcmp(key, "Signature"))
    {
      char* ptr = value;

      WORD w;
      if(getWord(w, value))
        form.signatureOffset = w;
      else
      {
        errors = true;
        continue;
      }
      int noQuestionMarks = 0;
      int i               = 0;
      
      for(; i < 32; ++i)
      {
        while(*ptr && !(*ptr == ' ' || *ptr == '\t')) ++ptr;
        ptr = skipWS(ptr);
        if(!*ptr) break; // кончились байтики

        if(*ptr != '?')
        {
          BYTE b;
          if(!getByte(b, ptr))
          {
            errors = true;
            break;
          }
          form.signature[i] = b;
          form.signatureMask |= 1<<(32-i);
        }
        else
          ++noQuestionMarks;
      }
      if(noQuestionMarks < i) form.signatureSize = i;
    }
  }
  CloseHandle(ini);
  if(checkFormat(form) && noFormats != 255) formats[noFormats++] = form;
  if(errors)
  {
    char *msgItems[3];
    msgItems[0] = getMsg(MxSCLWarning);
    msgItems[1] = getMsg(MINIError);
    msgItems[2] = getMsg(MOk);
    messageBox(FMSG_WARNING | FMSG_DOWN, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
  }
}

Detector::~Detector()
{
  for(int i = 0; i < noFormats; ++i)
    if(formats[i].description) delete[] formats[i].description;
}

BYTE Detector::detect(const FileHdr& hdr, const BYTE* secs, int size, char* comment)
{
  if(noFormats == 0) return 0xFF;
  *comment = 0;
  
  for(BYTE i = 0; i < noFormats; ++i)
  {
    bool detected = false;
    BYTE mask = 0;
    if(formats[i].type   ==  0 || hdr.type   == formats[i].type)   mask |= 0x01;
    if(formats[i].size   == -1 || hdr.size   == formats[i].size)   mask |= 0x02;
    if(formats[i].start  == -1 || hdr.start  == formats[i].start)  mask |= 0x04;
    if(formats[i].noSecs == -1 || hdr.noSecs == formats[i].noSecs) mask |= 0x08;
    
    if(formats[i].signatureSize == 0)
      mask |= 0x10;
    else
    {
      if(formats[i].signatureOffset + formats[i].signatureSize < size)
      {
        int n = 0;
        for(; n < formats[i].signatureSize; ++n)
          if((formats[i].signatureMask & 1<<(32-n)) &&
             secs[formats[i].signatureOffset+n] != formats[i].signature[n]) break;
        if(n == formats[i].signatureSize) mask |= 0x10;
      }
    }
    if(mask == 0x1F) detected = true;
    if(detected)
    {
      if(formats[i].commentOffset + formats[i].commentSize < size)
      {
        if(formats[i].commentSize)
        {
          int j = 0;
          int commentSize = formats[i].commentSize;
          if(formats[i].commentSize > 99) commentSize = 99;
          for(; j < commentSize; ++j)
          {
            BYTE b = secs[formats[i].commentOffset+j];
            if(b > 31 && b < 128)
              comment[j] = b;
            else
              comment[j] = ' ';
          }
          comment[j] = 0;
          trim(comment);
        }
      }
      char* ptr = comment;
      if(*ptr)
      {
        int len = lstrlen(ptr);
        char lastChar = ' ';
        
        for(int k = 0; k < len; ++k)
        {
          if(comment[k] != ' ')
          {
            *ptr++ = lastChar = comment[k];
          }
          else
          {
            if(lastChar != ' ') *ptr++ = lastChar = ' ';
          }
        }
        *ptr = 0;
      }
      return i;
    }
  }
  return 0xFF;
}
    
void Detector::specialChar(BYTE n, char *pos)
{
  if(n == 0xFF) return;
  if(formats[n].specialChar) *pos = formats[n].specialChar;
}

void Detector::getType(BYTE n, char *pos)
{
  if(n == 0xFF) return;
  if(formats[n].newType) *pos = formats[n].newType;
}

bool Detector::getSkipHeader(BYTE n)
{
  if(n == 0xFF) return 0;
  return formats[n].skipHeader;
}

char* Detector::description(BYTE n)
{
  if(n == 0xFF) return 0;
  return formats[n].description;
}
