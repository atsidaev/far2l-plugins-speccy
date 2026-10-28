#ifndef extract_hpp
#define extract_hpp

WORD getByte(BYTE& b);
bool putByte(BYTE b);

void explode (BYTE* from, int  fromSize, BYTE* to, int  toSize, bool smallFile, bool flag);
void unshrink(BYTE* from, WORD fromSize, BYTE* to, WORD toSize);
void unlz    (BYTE* from, WORD fromSize, BYTE* to, WORD toSize);

#endif
