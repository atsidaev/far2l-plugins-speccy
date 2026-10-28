#include <windows.h>
#include "../dtr.hpp"

namespace MSPACK
{

class MSPackStream
{
private:
	BYTE* base;
	BYTE* p;
	int   idx;
	int   len;
	bool  eof;
	WORD  bits;
public:
	MSPackStream(BYTE* from, int blockSize)
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

WORD calculateDisp(MSPackStream& s)
{
	if(s.getBit()) return s.getByte() + 1;

	BYTE code = s.getBits(3);
	if(code < 2) return 256*(code+1) + s.getByte() + 1;

	code = 2*code + s.getBit();
	if(code < 8) return 256*(code-1) + s.getByte() + 1;

	code = 2*code + s.getBit();
	if(code < 0x17) return 256*(code-9) + s.getByte() + 1;

	code = 2*code + s.getBit();
	if((code&0x1f) < 0x1f)return 256*(code&0x1f) + s.getByte() + 1;

	WORD disp = 256*s.getByte();
	disp += s.getByte() + 1;
	return disp;
}

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	WORD packed_size;
	BYTE *from = (BYTE*)source;
	BYTE *to   = unpackBuffer;

	if(source[0] == 'M' && source[1] == 's' && source[2] == 'P' && source[3] == 'k')
	{
		if(*(WORD*)(source + 0xE9) - *(WORD*)(source + 0xEB) !=5)
			packed_size = *(WORD*)(source + 0x08) + *(WORD*)(source + 0x0A) - *(WORD*)(source + 0x0C);
		else
			packed_size = *(WORD*)(source + 0x08) - *(WORD*)(source + 0x0C) + 1;
		from += 0x0E;
	}
	else
	{
		if(*(WORD*)(source + 0xE9) - *(WORD*)(source + 0xEB) !=5)
			packed_size = *(WORD*)(source + 0xED) + *(WORD*)(source + 0xEF) - *(WORD*)(source + 0xF1);
		else
			packed_size = *(WORD*)(source + 0xED) - *(WORD*)(source + 0xF1) + 1;
		from += 0xF3;
	}

	MSPackStream s(from, packed_size);

	while(!s.error())
	{
		while(!s.getBit()) *to++ = s.getByte();

		WORD len = 0;
		BYTE bb  = 0;

		do
		{
			bb = s.getBits(2);
			len += bb;
		} while(bb == 0x03 && len != 0x0f);

		len += 2;

		if(len == 2)
		{
			WORD disp = s.getByte() + 1;
			for(BYTE i = 0; i < 2; ++i) *to++ = to[-disp];
			continue;
		}

		if(len == 5)
		{
			BYTE b = s.getByte();
			if(b == 0xff) break; // EOF
			if(b == 0xfe)
				{ len = s.getByte(); len += 256*s.getByte(); }
			else
				len = b + 17;
		}
		else
			if(len > 5) --len;

		WORD disp = calculateDisp(s);

		for(WORD i = 0; i < len; ++i)
			*to++ = to[-disp];
	}
	for(BYTE i = 0; i < 5; ++i)
		*to++ = s.getByte();

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
	WORD size, packed_size;
	while(true)
	{
		if(data[0] == 'M' && data[1] == 's' && data[2] == 'P' && data[3] == 'k')
		{
			packed_size = *(WORD*)(data + 0x0A) + 3;
			if(*(WORD*)(data + 0x04) - *(WORD*)(data + 0x06) !=5)
				size = *(WORD*)(data + 0x08) + *(WORD*)(data + 0x0A) - *(WORD*)(data + 0x0C);
			else
				size = *(WORD*)(data + 0x08) - *(WORD*)(data + 0x0C) + 1;
			break;
		}

		if(data[0xE5] == 'M' && data[0xE6] == 's' && data[0xE7] == 'P' && data[0xE8] == 'k')
		{
			packed_size = *(WORD*)(data + 0xEF) + 3;
			if(*(WORD*)(data + 0xE9) - *(WORD*)(data + 0xEB) !=5)
				size = *(WORD*)(data + 0xED) + *(WORD*)(data + 0xEF) - *(WORD*)(data + 0xF1);
			else
				size = *(WORD*)(data + 0xED) - *(WORD*)(data + 0xF1) + 1;
			break;
		}
		return false;
	}

	if(packed_size > size || packed_size > file_size - sizeof(HoHdr)) return false;

	result.action = do_extract;
	strcpy(result.title, "[mspack 1.96]");

	return true;
}
}
