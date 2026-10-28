#include <windows.h>
#include "../dtr.hpp"

namespace CC3
{

const int depackerSize = 0x99;

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *from = (BYTE*)source + depackerSize;
	BYTE *to   = unpackBuffer;

	while((*from & 0x0f) != 0x03)
	{
		BYTE b = *from++;
		if((b & 0x01) == 0)  // backreference
		{
			BYTE len    = ((b & 0xf0)>>4) + 3;
			WORD offset = (128*(b&0x0f)) + *from++;

			for(BYTE i = 0; i < len; ++i) *to++ = to[-offset];

			if((b & 0xf0) == 0xf0)
			{
				BYTE len2 = *from++;
				if(len2 != 0)
					for(BYTE i = 0; i < len2; ++i) *to++ = to[-offset];
			}
			continue;
		}

		switch(b & 0x0f)
		{
		case 0x09: // short RLE [1001]
			{
				BYTE len = ((b & 0xf0)>>4) + 3;
				for(BYTE i = 0; i < len; ++i) *to++ = *from;
				++from;
				break;
			}
		case 0x01: // long RLE [0001]
			{
				WORD len = 256*((b&0xf0)>>4) + (*from++)+3;
				for(WORD i = 0; i < len; ++i) *to++ = *from;
				++from;
				break;
			}
		case 0x0B: // 2 bytes (00-0x0E,0xFF) [1011] {0011 - end}
			{
				BYTE bb = ((b&0xf0)>>4) - 1;
				*to++ = bb;
				*to++ = bb;
				break;
			}
		case 0x05: // short copy [0101]
			{
				BYTE len = ((b & 0xf0)>>4) + 1;
				for(BYTE i = 0; i < len; ++i) *to++ = *from++;
				break;
			}
		case 0x0d: // long copy [1101]
			{
				WORD len = 256*((b & 0xf0)>>4) + (*from++) + 1;
				for(WORD i = 0; i < len; ++i) *to++ = *from++;
				break;
			}
		default: // short backreference for 2 bytes [x111]
			{
				WORD offset = (b & 0xf8)>>3;
				*to++ = *(to-offset);
				*to++ = *(to-offset);
			}
		}
	}

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
	if(data[0] != 0xF3 || data[1] != 'K' || data[2] != 'S' || data[3] != 'A' || data[4] != 0x21)
		return false;

	WORD packedSize = data[23] + 256*data[24];
	if(packedSize > file_size) return false;

	// check end of stream
	if((data[packedSize + depackerSize - 1] & 0x0f) != 0x03) return false;

	result.action = do_extract;
	strcpy(result.title, "[Code Cruncher 3.11]");

	return true;
}
}
