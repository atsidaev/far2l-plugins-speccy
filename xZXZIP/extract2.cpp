#include <windows.h>
#include "extract.hpp"

#define HSIZE      8192          // HSIZE is defined as 2^13 (8192)
#define BOGUSCODE  256
#define FLAG_BITS  parent        // upper bits of parent[] used as flag bits
#define CODE_MASK  (HSIZE - 1)   // 0x1fff (lower bits are parent's index)
#define FREE_CODE  HSIZE         // 0x2000 (code is unused or was cleared)
#define HAS_CHILD  (HSIZE << 1)  // 0x4000 (code has a child--do not clear)

extern BYTE *curTo, *curFrom;
extern WORD toSize_, fromSize_;
extern WORD noBytesRead, noBytesWritten;

short parent[HSIZE];        // pointer to (8192 * sizeof(short))
BYTE  Value[HSIZE];         // pointer to 8KB char buffer
BYTE  stack[HSIZE];         // pointer to another 8KB char buffer

//**************************
// Function partial_clear()        // no longer recursive...
//**************************
void partial_clear()
{
  short code;

  // clear all nodes which have no children (i.e., leaf nodes only)

  // first loop:  mark each parent as such
  for(code = BOGUSCODE+1; code < HSIZE; ++code)
  {
    short cparent = (short)(parent[code] & CODE_MASK);
    if(cparent > BOGUSCODE && cparent != FREE_CODE)
      FLAG_BITS[cparent] |= HAS_CHILD;   // set parent's child-bit
  }

  // second loop:  clear all nodes *not* marked as parents; reset flag bits
  for(code = BOGUSCODE+1; code < HSIZE; ++code)
  {
    if(FLAG_BITS[code] & HAS_CHILD)    // just clear child-bit
      FLAG_BITS[code] &= ~HAS_CHILD;
    else                                // leaf:  lose it
      parent[code] = FREE_CODE;
  }
}

bool  zipEOF;
DWORD bhold;
WORD  bsize;

void getCode(short& code, WORD bits)
{
  static int bit_mask[] =
  {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f,
    0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff,
    0x1fff, 0x3fff, 0x7fff, 0xffff
  };

  if(bits > 13 || bits == 0) {
    zipEOF = true;
    return;
  }
  if(bits > bsize)   // fill bhold, 8*sizeof(DWORD) bits
  {
    BYTE temp;

    zipEOF = 1;
    while (bsize <= 8*(int)(sizeof(bhold)-1) && getByte(temp))
    {
      bhold |= (DWORD)temp << bsize;
      bsize += 8;
      zipEOF = 0;
    }
  }
  if(bits > bsize) {
    zipEOF = true;
    return;
  }
  zipEOF = false;
  code = (short)((WORD)bhold & bit_mask[bits]);
  bhold >>= bits;
  bsize -= bits;
}

void unshrink(BYTE* from, WORD fromSize, BYTE* to, WORD toSize)
{
  bsize = 0;
  bhold = 0;

  curTo     = to;
  curFrom   = from;
  fromSize_ = fromSize;
  toSize_   = toSize;

  noBytesRead = 0; noBytesWritten = 0;

  int offset = (HSIZE - 1);
  BYTE *stacktop = stack + offset;
  BYTE * newstr;
  int codesize = 9, len;
  bool KwKwK;
  short code, oldcode, freecode, curcode;
  short lastfreecode;

  //---------------------------------------------------------------------------
  //  Initialize various variables.
  //---------------------------------------------------------------------------

  lastfreecode = BOGUSCODE;

  for(code = 0; code < BOGUSCODE; ++code)
  {
    Value[code] = (BYTE)code;
    parent[code] = BOGUSCODE;
  }
  for(code = BOGUSCODE+1; code < HSIZE; ++code)
    parent[code] = FREE_CODE;

  //---------------------------------------------------------------------------
  //  Get and output first code, then loop over remaining ones.
  //---------------------------------------------------------------------------

  getCode(oldcode, codesize);
  if(zipEOF || oldcode < 0 || oldcode >= 256)
    return;
  if(!putByte((BYTE)oldcode))
    return;

  do
  {
    getCode(code, codesize);
    if(zipEOF) break;
    if(code == BOGUSCODE)    // possible to have consecutive escapes?
    {
      getCode(code, codesize);
      if(zipEOF)
        return;
      if(code == 1) {
        if(++codesize > 13)
          return;
      } else if(code == 2) {
        partial_clear();          // clear leafs (nodes with no children)
        lastfreecode = BOGUSCODE; // reset start of free-node search
      }
      continue;
    }

    //-----------------------------------------------------------------------
    //  Translate code:  traverse tree from leaf back to root.
    //-----------------------------------------------------------------------

    newstr = stacktop;
    curcode = code;

    if(curcode < 0 || curcode >= HSIZE)
      return;
    if(parent[curcode] == FREE_CODE) // or (FLAG_BITS[curcode] & FREE_CODE)?
    {
      KwKwK = true;
      --newstr;   // last character will be same as first character
      curcode = oldcode;
    }
    else
      KwKwK = false;
    do
    {
      if(curcode < 0 || curcode >= HSIZE || newstr <= stack)
        return;
      *newstr-- = Value[curcode];
      curcode = (short)(parent[curcode] & CODE_MASK);
    } while(curcode != BOGUSCODE);

    len = (int)(stacktop - newstr++);
    if(KwKwK)
      *stacktop = *newstr;

    //-----------------------------------------------------------------------
    //  Write expanded string in reverse order to output buffer.
    //-----------------------------------------------------------------------

    {
      BYTE * p;
      for(p = newstr; p < newstr+len; ++p)
        if(!putByte(*p)) return;
    }

    //-----------------------------------------------------------------------
    //  Add new leaf (first character of newstr) to tree as child of oldcode.
    //-----------------------------------------------------------------------

    // search for freecode
    freecode = (short)(lastfreecode + 1);
    // add if-test before loop for speed?
    while(freecode < HSIZE && parent[freecode] != FREE_CODE)
      ++freecode;
    if(freecode == HSIZE)
      return;
    lastfreecode = freecode;

    Value[freecode] = *newstr;
    parent[freecode] = oldcode;
    oldcode = code;

  } while(!zipEOF);
}
