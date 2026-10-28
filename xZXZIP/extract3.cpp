#include <windows.h>
#include <stdlib.h>  // для qsort
#include <string.h>  // для strncmp
#include "extract.hpp"

extern BYTE *curTo, *curFrom;
extern WORD toSize_, fromSize_;
extern WORD noBytesRead, noBytesWritten;

// один элемент дерева Шенона-Фано
struct  SFTEntry
{
  BYTE blen;         // кол-во бит
  BYTE value;        // значение
  WORD code;         // код
};

// упакованные деревья Шенона-Фано
BYTE pSFT_64_1[] =
{
 0x07, // размер
 0x01, 0x13, 0x34, 0xE5, 0xF6, 0x96, 0xF7 // упакованные данные
};

BYTE pSFT_64_2[] =
{
 0x10,
 0x00, 0x12, 0x03, 0x24, 0x15, 0x36, 0x27,
 0x38, 0x39, 0x6A, 0x7B, 0x4C, 0x9D, 0x6E,
 0x1F, 0x09
};

BYTE pSFT_64_3[] =
{
 0x07,
 0x12, 0x23, 0x14, 0xE5, 0xF6, 0x96, 0xF7
};

BYTE pSFT_64_4[] =
{
 0x0D,
 0x01, 0x22, 0x23, 0x14, 0x15, 0x36, 0x37,
 0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05
};

BYTE pSFT_64_5[] =
{
 0x07,
 0x12, 0x13, 0x44, 0xC5, 0xF6, 0x96, 0xF7
};

BYTE pSFT_64_6[] =
{
 0x0E,
 0x02, 0x01, 0x12, 0x23, 0x14, 0x15, 0x36,
 0x37, 0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05
};

BYTE pSFT_100[] =
{
 0x62,
 0x0A, 0x7B, 0x07, 0x06, 0x1B, 0x06, 0xBB,
 0x0C, 0x4B, 0x03, 0x09, 0x07, 0x0B, 0x09,
 0x0B, 0x09, 0x07, 0x16, 0x07, 0x08, 0x06,
 0x05, 0x06, 0x07, 0x06, 0x05, 0x36, 0x07,
 0x16, 0x17, 0x0B, 0x0A, 0x06, 0x08, 0x0A,
 0x0B, 0x05, 0x06, 0x15, 0x04, 0x06, 0x17,
 0x05, 0x0A, 0x08, 0x05, 0x06, 0x15, 0x06,
 0x0A, 0x25, 0x06, 0x08, 0x07, 0x18, 0x0A,
 0x07, 0x0A, 0x08, 0x0B, 0x07, 0x0B, 0x04,
 0x25, 0x04, 0x25, 0x04, 0x0A, 0x06, 0x04,
 0x05, 0x14, 0x05, 0x09, 0x34, 0x07, 0x06,
 0x17, 0x09, 0x1A, 0x2B, 0xFC, 0xFC, 0xFC,
 0xFB, 0xFB, 0xFB, 0x0C, 0x0B, 0x2C, 0x0B,
 0x2C, 0x0B, 0x3C, 0x0B, 0x2C, 0x2B, 0xAC
};

int exp_cmp(const void *a, const void *b)
{
  return memcmp(a, b, 2); // compare blen and value
}

void extractSFTree(BYTE *ptr, SFTEntry *sft, int n)
{
  WORD noBytesToRead = *ptr++;

  int i = 0;
  while(noBytesToRead--) // читаем дерево
  {
    WORD blen = (*ptr & 0x0f) + 1; // получили кол-во бит
    WORD ne   = (*ptr++ >> 4) + 1; // получили кол-во значений

    while(ne--)
    {
     sft[i].blen  = blen;
     sft[i].value = i;
     ++i;
    }
  }
  
  qsort((void*) sft, n, sizeof(SFTEntry), exp_cmp);

  WORD code = 0, lastBitLen = 0, codeIncrement = 0;
  
  SFTEntry *p;
  for(p = &sft[n - 1]; p >= sft; p--) // строим коды
  {
    code += codeIncrement;
    p->code = code;
    if(p->blen != lastBitLen)
    {
      lastBitLen    = p->blen;
      codeIncrement = (1 << (16 - lastBitLen));
    }
  }
  
  p = sft;
  while(n--) // инвертируем коды
  {
    code              = p->code;
    WORD invertedCode = 0;

    for(int i = 16; i--;)
    {
      invertedCode = (invertedCode << 1) | (code & 1);
      code >>= 1;
    }
    p->code = invertedCode;
    ++p;
  }
}

static DWORD bhold;
static int   bsize;

bool getCode(WORD& code, int bits)
{
  static WORD bit_mask[] =
  {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f,
    0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff,
    0x1fff, 0x3fff, 0x7fff, 0xffff
  };

  BYTE b;
  while(bsize < bits)
  {
    if(!getByte(b)) return false;
    bhold |= (DWORD)b << bsize;
    bsize += 8;
  }
  code = ((WORD) bhold) & bit_mask[bits];
  bhold >>= bits;
  bsize -= bits;

  return true;
}

bool expRead(WORD & result, SFTEntry * sf, size_t count) {
  SFTEntry* p = sf;
  WORD      c;
  WORD      code = 0, bits =0;
  for(;;)
  {
    if(bits >= 16 || !getCode(c, 1))
      return false;
    c <<= bits++;
    code |= c;

    for(;;)
    {
      if(p == sf + count)
        return false;
      if(p->blen > bits) break;
      if(p->blen == bits && p->code == code)
      {
        result = p->value;
        return true;
      }
      p++;
    }
  }
}

void explode(BYTE* from, int fromSize, BYTE* to, int toSize, bool smallFile, bool flag)
{
  SFTEntry sft1[256];
  SFTEntry sft2[64];
  SFTEntry sft3[64];

  int minMatchLen    = 2;
  int noDistanceBits = 6;
  
  if(smallFile)
  {
    if(flag)
    {
      extractSFTree(pSFT_64_6, sft2, 64);
      extractSFTree(pSFT_64_5, sft3, 64);
    }
    else
    {
      extractSFTree(pSFT_64_2, sft2, 64);
      extractSFTree(pSFT_64_1, sft3, 64);
    }
  }
  else
  {
    if(flag)
    {
      minMatchLen = 3;
      noDistanceBits = 7;
      extractSFTree(pSFT_100,  sft1, 256);
      extractSFTree(pSFT_64_4, sft2, 64);
      extractSFTree(pSFT_64_3, sft3, 64);
    }
    else
    {
      extractSFTree(pSFT_64_2, sft2, 64);
      extractSFTree(pSFT_64_1, sft3, 64);
    }
  }

  bhold = 0;
  bsize = 0;

  curTo     = to;
  curFrom   = from;
  fromSize_ = fromSize;
  toSize_   = toSize;

  noBytesRead = 0; noBytesWritten = 0;
  
  WORD code, dist, len;
  for(;;)
  {
    if(!getCode(code, 1)) break;
    if(code)
    {
      if(minMatchLen == 3)
      {
        if(!expRead(code, sft1, 256))
          break;
      }
      else
      {
        if(!getCode(code, 8)) break;
      }
      if(!putByte(code)) return;
    }
    else
    {
      if(!getCode(dist, noDistanceBits)) break;
      if(!expRead(code, sft3, 64))
        break;

      dist = (dist | (code << noDistanceBits)) + 1;

      if(!expRead(len, sft2, 64))
        break;
      if(len == 63)
      {
        if(!getCode(code, 8)) break;
        len += code;
      }
      
      len += minMatchLen;
      int previous = int(noBytesWritten) - dist;
      BYTE b;
      while (len--)
      {
        if(previous < 0)
          b = 0;
        else
          b = to[previous];
        if(!putByte(b)) return;
        ++previous;
      }
    }
  }
}
