#include <windows.h>
#include "../dtr.hpp"

namespace PCD
{

class BBStream
{
private:
	BYTE* base;
	BYTE* p;
	int   idx;
	int   len;
	bool  eof;
	WORD  bits;
public:
	BBStream(BYTE* from, int blockSize)
	{
		base = p = from;

		len = blockSize;
		idx = 0;
		eof = false;
	}

	BYTE getByte(void)
	{
		if(p - base == len) { eof = true; return 0; }
		return *p++;
	}

	BYTE getBit()
	{
		WORD mask[]  = { 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 };
		if(idx == 0)
		{
			bits  = getByte();
			bits += 256*getByte();
		}

		BYTE bit = (bits & mask[idx]) ? 1 : 0;

		idx = (idx + 1) % 16;
		return bit;
	}

	BYTE getBits(int n)
	{
		BYTE r = 0;
		do { r = 2*r + getBit(); } while(--n);
		return r;
	}

	bool error(void) { return eof; }
};

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD internal_id)
{
	BYTE *from = (BYTE*)source + ((internal_id == 5) ? 0xc4+5: 0xc9+3);
	BYTE *to   = unpackBuffer;

	BBStream s(from, sourceSize);

	while(!s.error())
	{
		while(s.getBit()) *to++ = s.getByte();

		WORD len = 0;
		BYTE bb  = 0;
		do
		{
			bb = s.getBits(2);
			len += bb;
		} while(bb == 0x03 && len != 0x0f);

		if(len == 0)
		{
			BYTE offset = s.getBits(4) + 1;
			*to++ = to[-offset];
			continue;
		}

		if(len == 1)
		{
			WORD offset = s.getByte();
			if(offset == 0xff) break;
			++offset;
			for(BYTE i = 0; i < 2; ++i)
				*to++ = to[-offset];
			continue;
		}

		if(len == 3)
		{
			len = s.getByte();
			if(len == 0)
			{
				len  = s.getByte();
				len += 256*s.getByte();
			}
			else
				len += 0x0f;
		}

		if(len == 2) len = 3;

		WORD offset = s.getByte();
		BYTE high   = 0;
		if(s.getBit())
		{
			do { high = 4*high + s.getBits(2); } while(s.getBit());
			++high;
		}
		offset += 256*high;
		for(WORD i = 0; i < len; ++i)
			*to++ = to[-offset];
	}
	for(int i = 0; i < internal_id; ++i)
		*to++ = from[i - internal_id];

	if(s.error())
		return false;

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
	int i;
	bool depackerFound = true;

	BYTE signature[] = {0x11, 0x03, 0x00, 0x3e, 0x80, 0x10, 0x02, 0xe1};
	for(i = 0; i < sizeof(signature); ++i)
		if(data[0x3b+i] != signature[i]) { depackerFound = false; break; }

	if(depackerFound)
	{
		WORD packedSize   = data[0x1a] + 256*data[0x1b];
		WORD depackerSize = 0xc4;

		if(packedSize <= file_size && data[packedSize+depackerSize+5-1] == 0xff)
		{
			result.internal_id = 5;
			result.action      = do_extract;
			strcpy(result.title, "[Powerfull Code Decreaser 6.2]");
			return true;
		}
	}

	depackerFound = true;

	for(i = 0; i < sizeof(signature); ++i)
		if(data[0x43+i] != signature[i]) { depackerFound = false; break; }

	if(depackerFound)
	{
		WORD packedSize   = data[0x14] + 256*data[0x15];
		WORD depackerSize = 0xc9;

		if(packedSize <= file_size && data[packedSize+depackerSize-1] == 0xff)
		{
			result.internal_id = 3;
			result.action      = do_extract;
			strcpy(result.title, "[Powerfull Code Decreaser 6.1]");
			return true;
		}
	}

	return false;
}
}
