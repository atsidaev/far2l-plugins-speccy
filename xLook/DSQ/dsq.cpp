#include <windows.h>
#include "../dtr.hpp"

namespace DSQ
{

const int depackerSize = 0xaf;

class BitStream
{
private:
	BYTE* base;
	BYTE* p;
	int   idx;
	int   len;
	bool  eof;
	BYTE  bits;

	BYTE getByte()
	{
		if(base - p >= len) { eof = true; return 0; }
		return *p--;
	}

public:
	BitStream(BYTE* from, int blockSize)
	{
		base = p = from + blockSize;

		len  = blockSize;
		idx  = 0;
		eof  = false;
	}

	BYTE getBit()
	{
		BYTE mask[] = { 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 };

		if(idx == 0) bits = getByte();
		BYTE bit = (bits & mask[idx]) ? 1 : 0;

		idx = (idx+1)%sizeof(mask);
		return bit;
	}
	WORD getBits(int n)
	{
		WORD r = 0;
		do { r = 2*r + getBit(); } while(--n);
		return r;
	}
	bool error(void) { return eof; }
};

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *to = unpackBuffer + *noUnpackedBytes - 1;

	WORD packedSize   = source[0x13] + 256*source[0x14];
	WORD unpackedSize = source[0x21] + 256*source[0x22] - source[0x8e] - 256*source[0x8f];

	BitStream s((BYTE*)source + depackerSize, packedSize);

	BYTE noBits = source[0x82];

	while(!s.error())
	{
		if(to <= unpackBuffer + *noUnpackedBytes - unpackedSize) break;

		if(!s.getBit())
		{
			*to-- = s.getBits(8);
			continue;
		}

		WORD len = 0;
		if(s.getBit()) // %11
			len = 2 + s.getBit();
		else
		{
			if(s.getBit()) // %101
				len = 4 + s.getBits(2);
			else
			{
				if(s.getBit()) // %1001
				{
					len = 8 + s.getBits(4);
					if(len == 0x17)
					{
						len = 0x0e + s.getBits(5);
						while(len--) *to-- = s.getBits(8);
						continue;
					}
				}
				else // %1000
				{
					len    = 0x17;
					BYTE b = 0;
					do { b = s.getBits(8); len += b; } while(b == 0xff);
				}
			}
		}
		WORD offset = 0;
		if(s.getBit()) // %1
			offset = 0x221 + s.getBits(noBits);
		else
		{
			if(s.getBit()) // %01
				offset = 1 + s.getBits(5);
			else // %00
				offset = 0x21 + s.getBits(9);
		}

		for(WORD i = 0; i < len; ++i) *to-- = to[offset];
	}
	if(s.error()) return false;

	*noUnpackedBytes = *noUnpackedBytes-(to-unpackBuffer)-1;
	memmove(unpackBuffer, unpackBuffer+(to-unpackBuffer)+1, *noUnpackedBytes);

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
	BYTE signature[] = {0xC6, 0x08, 0x6F, 0xFE, 0x17, 0x38, 0x1F, 0x06, 0x05};
	for(int i = 0; i < sizeof(signature); ++i)
		if(data[0x43+i] != signature[i]) return false;

	WORD packedSize = data[0x13] + 256*data[0x14];
	if(packedSize > file_size) return false;

	result.action = do_extract;
	strcpy(result.title, "[Data Squeezer 4.12]");

	return true;
}
}
