#include <windows.h>
#include "../dtr.hpp"

namespace LZS
{

const int depackerSize = 0x82;

bool WINAPI extract(HoHdr*, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *from = (BYTE*)source + depackerSize;
	BYTE *to   = unpackBuffer;

	while(*from != 0x80)
	{
		BYTE b = *from++;
		BYTE i;
		switch(b & 0xc0)
		{
		case 0x80: // copy
			for(i = 0; i < (b & 0x3f); ++i) *to++ = *from++;
			break;
		case 0xc0: // RLE
			for(i = 0; i < (b & 0x3f)+3; ++i) *to++ = *from;
			++from;
			break;
		default: // backreference (LZ)
			{
				BYTE len    = ((b & 0xf0)>>4) + 3;
				WORD offset = (256*(b&0x0f)) + *from++;
				for(BYTE i = 0; i < len; ++i) *to++ = *(to-offset);
			}
		}
	}

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
	BYTE signature[] = {0x06, 0, 0x7E, 0xCB, 0x7F, 0x20, 0x1D, 0xE6, 0x0F, 0x47};
	for(int i = 0; i < sizeof(signature); ++i)
		if(data[38+i] != signature[i]) return false;

	WORD packedSize = data[27] + 256*data[28];
	if(packedSize > file_size) return false;

	//if(data[sizeof(HoHdr)+depackerSize+packedSize-2] != 0x80) return false;

	result.action = do_extract;
	strcpy(result.title, "[LZS]");

	return true;
}
}
