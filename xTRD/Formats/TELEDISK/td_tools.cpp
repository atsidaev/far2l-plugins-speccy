#include <windows.h>
#include "td_tools.hpp"
#include "td_types.hpp"
#include "teledisk.hpp"
#include "../../../shared/widestring.hpp"

HANDLE   td;
char     tdFileName[300];
bool     isPacked;
ImageHdr imgHdr;

WORD readCacheIndex;
WORD readCacheLen;
BYTE readCache[32768];

WORD calculateCRC16(BYTE* buf, WORD size)
{
  WORD crc = 0;
  while(size--) crc = (crc>>8) ^ crcTab[(crc&0xff) ^ *buf++];
  return ((crc&0xff)<<8) | (crc>>8);
}

// переход к следующему тому
bool nextVolume(void)
{
  CloseHandle(td);
  // меняем расширение
  tdFileName[strlen(tdFileName)-1]++;
  td = CreateFile(_W(tdFileName).c_str(),
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL,
                  OPEN_EXISTING,
                  FILE_FLAG_SEQUENTIAL_SCAN,
                  NULL);
  if(td == INVALID_HANDLE_VALUE) return false;

  ImageHdr tmpHdr;
  DWORD tmp;
  
  ReadFile(td, &tmpHdr, sizeof(ImageHdr), &tmp, NULL);
  if(tmp != sizeof(ImageHdr)) return false;
  if(tmpHdr.crc == calculateCRC16((BYTE*)&tmpHdr, sizeof(ImageHdr)-2) &&
     tmpHdr.id[0]    == imgHdr.id[0] &&
     tmpHdr.id[1]    == imgHdr.id[1] &&
     tmpHdr.volumeId == imgHdr.volumeId &&
     tmpHdr.volume   == imgHdr.volume + 1)
  {
    memcpy(&imgHdr, &tmpHdr, sizeof(ImageHdr));
    return true;
  }
  else
  {
    CloseHandle(td);
    return false;
  }
}

bool readBlock(BYTE* buf, WORD size, WORD* realSize)
{
  while(1)
  {
    DWORD noBytesRead;
    ReadFile(td, buf, size, &noBytesRead, NULL);
    *realSize = noBytesRead;
    
    if(*realSize == size)
      return true;
    else
    {
      buf  += *realSize;
      size -= *realSize;
      if(!nextVolume()) return false;
    }
  }
}

short readChar(void)
{
  if(readCacheIndex == readCacheLen)
  {
    readBlock(readCache, 32768UL, &readCacheLen);
    if(readCacheLen)
    {
      readCacheIndex=1;
      return readCache[0];
    }
    else
    {
      readCacheIndex=0;
      return -1;
    }
  }
  else
    return readCache[readCacheIndex++];
}

short r;

void StartHuff(void)
{
  short i, j;
  
  for (i = 0; i < N_CHAR; i++) {
    freq[i] = 1;
    son[i] = i + T;
    prnt[i + T] = i;
  }
  i = 0; j = N_CHAR;
  while (j <= R) {
    freq[j] = freq[i] + freq[i + 1];
    son[j] = i;
    prnt[i] = prnt[i + 1] = j;
    i += 2; j++;
  }
  freq[T] = 0xffff;
  prnt[R] = 0;
  
  for (i = 0; i < N - F; i++) text_buf[i] = ' ';
  r = N - F;
}

WORD getbuf;
BYTE  getlen;

short GetBit(void)	/* get one bit */
{
  short i;
  
  while(getlen <= 8)
  {
    if((i = readChar()) == -1) i = 0;
    getbuf |= i << (8 - getlen);
    getlen += 8;
  }
  i = getbuf;
  getbuf <<= 1;
  getlen--;
  return (i < 0);
}

short GetByte(void)	/* get one byte */
{
  unsigned i;
  
  while (getlen <= 8)
  {
    if((i = readChar()) == -1) i = 0;
    getbuf |= i << (8 - getlen);
    getlen += 8;
  }
  i = getbuf;
  getbuf <<= 8;
  getlen -= 8;
  return i >> 8;
}

/* reconstruction of tree */
void reconst(void)
{
  short i, j, k;
  WORD f, l;
  
  /* collect leaf nodes in the first half of the table */
  /* and replace the freq by (freq + 1) / 2. */
  j = 0;
  for(i = 0; i < T; i++)
  {
    if(son[i] >= T)
    {
      freq[j] = (freq[i] + 1) / 2;
      son[j] = son[i];
      j++;
    }
  }
  /* begin constructing tree by connecting sons */
  for(i = 0, j = N_CHAR; j < T; i += 2, j++)
  {
    k = i + 1;
    f = freq[j] = freq[i] + freq[k];
    for(k = j - 1; f < freq[k]; k--);
    k++;
    l = (j - k) * 2;
    memcpy(&freq[k + 1], &freq[k], l);
    freq[k] = f;
    memcpy(&son[k + 1], &son[k], l);
    son[k] = i;
  }
  /* connect prnt */
  for (i = 0; i < T; i++)
  {
    if ((k = son[i]) >= T)
    {
      prnt[k] = i;
    }
    else
    {
      prnt[k] = prnt[k + 1] = i;
    }
  }
}

/* increment frequency of given code by one, and update tree */

void update(int c)
{
  short i, j, k, l;
  
  if(freq[R] == MAX_FREQ) reconst();
  
  c = prnt[c + T];
  do
  {
    k = ++freq[c];
    
    /* if the order is disturbed, exchange nodes */
    if (k > freq[l = c + 1])
    {
      while (k > freq[++l]);
      l--;
      freq[c] = freq[l];
      freq[l] = k;
      
      i = son[c];
      prnt[i] = l;
      if (i < T) prnt[i + 1] = l;
      
      j = son[l];
      son[l] = i;
      
      prnt[j] = c;
      if (j < T) prnt[j + 1] = c;
      son[c] = j;
      
      c = l;
    }
  }while ((c = prnt[c]) != 0);	/* repeat up to root */
}

int DecodeChar(void)
{
  WORD c;
  
  c = son[R];
  
  /* travel from root to leaf, */
  /* choosing the smaller child node (son[]) if the read bit is 0, */
  /* the bigger (son[]+1} if 1 */
  while(c < T)
  {
    c += GetBit();
    c = son[c];
  }
  c -= T;
  update(c);
  return c;
}

int DecodePosition(void)
{
  WORD i, j, c;

  /* recover upper 6 bits from table */
  i = GetByte();
  c = (WORD)d_code[i] << 6;
  j = d_len[i];
  
  /* read lower 6 bits verbatim */
  j -= 2;
  while (j--)
  {
    i = (i << 1) + GetBit();
  }
  return c | (i & 0x3f);
}

WORD unLZH_internal(BYTE* buf, WORD textsize)
{
  short  i, j, k, c;
  WORD  count;
  
  for(count = 0; count < textsize; )
  {
    c = DecodeChar();
    if(c < 256)
    {
      *buf++ = c;
      text_buf[r++] = c;
      r &= (N - 1);
      count++;
    }
    else
    {
      i = (r - DecodePosition() - 1) & (N - 1);
      j = c - 255 + THRESHOLD;
      for (k = 0; k < j; k++)
      {
        c = text_buf[(i + k) & (N - 1)];
        *buf++ = c;
        text_buf[r++] = c;
        r &= (N - 1);
        count++;
      }
    }
  }
  return count;
}

BYTE  tmpBuf[32*257+F];
WORD  oldSize;
BYTE* oldPtr;

bool unLZH(BYTE* buf, WORD size)
{
  if(oldSize < size)
  {
    WORD realSize = unLZH_internal(oldPtr, size);
    oldSize += realSize;
  }
  memcpy(buf, tmpBuf, size);
  oldSize -= size;
  memcpy(tmpBuf,  tmpBuf+size, oldSize);
  oldPtr = tmpBuf+oldSize;
  
  return true;
}

bool read_(BYTE* buf, WORD size)
{
  WORD tmpVar;
  if(isPacked)
    return unLZH(buf, size);
  else
    return readBlock(buf, size, &tmpVar);
}

bool unRLE (BYTE* packedSec, short secSize, BYTE* sec)
{
  switch(packedSec[0])
  {
    case 0:
              memcpy(sec, packedSec+1, secSize-1);
              return true;
    case 1:
              {
                WORD noWords = *(WORD*)(packedSec+1);
                BYTE lo = packedSec[3];
                BYTE hi = packedSec[4];
                for(WORD i = 0; i < noWords; ++i)
                {
                  sec[2*i]   = lo;
                  sec[2*i+1] = hi;
                }
              }
              return true;
    case 2:
              {
                BYTE* from = packedSec+1;
                BYTE* to = sec;
                secSize--;
                do
                {
                  BYTE type = *from++;
                  --secSize;
                  if(type == 0)
                  {
                    BYTE noBytes = *from++;
                    --secSize;
                    secSize -= noBytes;
                    while(noBytes--) *to++ = *from++;
                  }
                  if(type == 1)
                  {
                    BYTE noWords = *from++;
                    --secSize;
                    BYTE lo = *from++;
                    BYTE hi = *from++;
                    secSize -= 2;
                    while(noWords--)
                    {
                      *to++ = lo;
                      *to++ = hi;
                    }
                  }
                  if(type > 1) return false;
                } while(secSize > 0);
              }
              return true;
    default:
              return false;
  }
}

void td_init(HANDLE hostFile, const char* fileName, ImageHdr imgHdr_, bool isPacked_)
{
  td = hostFile;
  strcpy(tdFileName, fileName);
  imgHdr = imgHdr_;
  isPacked = isPacked_;

  readCacheIndex = readCacheLen = 0;
  oldSize = 0;
  oldPtr = tmpBuf;
  getbuf = 0;
  getlen = 0;
  if(isPacked) StartHuff();
}
