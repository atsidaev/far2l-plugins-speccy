#include <windows.h>
#include "../dtr.hpp"

namespace TRUSH
{

const int depackerSize = 0x119;

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
		bits  = getByte();
		bits += 256*getByte();
	}

	BYTE getByte(void)
	{
		if(p - base == len) { eof = true; return 0; }
		return *p++;
	}

	BYTE getBit()
	{
		WORD mask[]  = { 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 };

		BYTE bit = (bits & mask[idx]) ? 1 : 0;

		if(idx == 15)
		{
			bits  = getByte();
			bits += 256*getByte();
		}

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

WORD calculateOffset(BBStream& s)
{
	if(s.getBit()) return s.getByte() + 1;

	BYTE code = s.getBits(3);
	if(code < 2) return 256*(code+1) + s.getByte() + 1;

	code = 2*code + s.getBit();
	if(code < 8) return 256*(code-1) + s.getByte() + 1;

	code = 2*code + s.getBit();
	if(code < 0x17) return 256*(code-9) + s.getByte() + 1;

	code = 2*code + s.getBit();
	return 256*(code&0x1f) + s.getByte() + 1;
}

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *from = (BYTE*)source + depackerSize;
	BYTE *to   = unpackBuffer;

	BBStream s(from, sourceSize);

	while(!s.error())
	{
		while(!s.getBit()) *to++ = s.getByte();

		BYTE code = s.getBits(2);

		if(code != 0)
		{
			code = 2*code + s.getBit();
			if(code >= 6) code = 2*code + s.getBit();
		}

		if(code == 2)
		{
			WORD offset = s.getByte() + 1;
			for(BYTE i = 0; i < 2; ++i) *to++ = to[-offset];
			continue;
		}

		WORD len = 0;
		if(code < 2)  len = code + 3;

		if(code == 3)
		{
			BYTE a = s.getByte();
			if(a == 0xff) break; // END of STREAM

			if(a == 0xfe)
			{
				len  = s.getByte();
				len += 256*s.getByte();
			}
			else
				len = a + 10;
		}

		if(code > 3)
		{
			len = code;
			if(len >= 6) len -= 6;
		}

		WORD offset = calculateOffset(s);
		for(WORD i = 0; i < len; ++i) *to++ = to[-offset];
	}
	if(s.error()) return false;

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
	BYTE signature[] = {'C','O','M','P','R','E','S','S','O','R',' ','B','Y',' ','A','L','E','X','A','N','D','E','R',' ','T','R','U','S','H',' ','O','D','E','S','S','A'};
	for(int i = 0; i < sizeof(signature); ++i)
		if(data[0x27+i] != signature[i]) return false;

	WORD packedSize = data[12] + 256*data[13];
	if(packedSize > file_size) return false;

	// check end of stream
	if(data[packedSize+depackerSize-1] != 0xff) return false;

	result.action = do_extract;
	strcpy(result.title, "[Trush Compressor]");

	return true;
}
}
