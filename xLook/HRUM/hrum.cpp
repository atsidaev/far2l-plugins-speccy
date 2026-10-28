#include <windows.h>
#include "../dtr.hpp"

namespace HRUM
{

BYTE *hr_d_output;
BYTE *hr_d_input;
WORD  hr_d_tagBYTE;/*буфер для битов.*/
BYTE  hr_d_tagcalc;/*счетчик этих битов*/

void hr_d_getbitinit(void)
{
	hr_d_tagBYTE = *hr_d_input++;
	hr_d_tagBYTE = hr_d_tagBYTE + ((*hr_d_input++) << 8);
	hr_d_tagcalc = 0x10;
}

BYTE hr_d_getbit(void)
{
	WORD tmp;

	tmp = hr_d_tagBYTE & 0x8000;
	hr_d_tagBYTE <<= 1;
	hr_d_tagcalc--;

	if(!hr_d_tagcalc)
		hr_d_getbitinit();
	return (tmp >> 15);
}

WORD hr_d_getlen(void)
{
	WORD tmp, result = 1;
	do
	{
		tmp = (hr_d_getbit() << 1) + hr_d_getbit();
		result += tmp;
	} while(tmp == 0x03 && result != 0x10);

	return(result);
}

WORD hr_d_getdist(void)
{
	WORD result, tmp;

	if(!hr_d_getbit())
		result = 0xff00 + *hr_d_input++;
	else
	{
		result = 0xffff;
		for(tmp=4; tmp; tmp--) result = (result << 1) + hr_d_getbit();
		result = (result << 8) + *hr_d_input++;
	}
	return (result);
}

void dehrum(BYTE* source, BYTE* destination)
{
	BYTE  tmpbuf[5];
	WORD  len;
	short offs;

	hr_d_input   = source /*+ 0x91*/;
	hr_d_output  = destination;

	for(int i = 0; i < 5; i++) tmpbuf[i] = *hr_d_input++;
	hr_d_input+=2;/*пропустили 2 байта 0x1010 - инициализация переменных*/
	hr_d_getbitinit();/*взяли 16 битов в буфер*/

	*hr_d_output++ = *hr_d_input++;

	bool done = false;
	while(!done)
	{
		if(!hr_d_getbit())
		{
			len = hr_d_getlen();
			offs = 0xffff;

			if(len == 4)
			{
				len = *hr_d_input++;

				if(len == 0)
					done = true;
				else
				{
					offs = hr_d_getdist();
					for(; len; len--)  *hr_d_output = *(hr_d_output++ + offs);
				}
			}
			else
			{
				if(len > 4) len--;
				if(len == 1)
					for(int i = 3; i; i--) offs = (offs << 1) + hr_d_getbit();
				else
				{
					if(len == 2)
						offs = 0xff00 + *hr_d_input++;
					else
						offs = hr_d_getdist();
				}
				for(; len; len--) *hr_d_output = *(hr_d_output++ + offs);
			}
		}
		else
			*hr_d_output++ = *hr_d_input++;
	}

	for(int i = 0; i < 5; i++) *hr_d_output++ = tmpbuf[i];
}

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	*noUnpackedBytes = *(WORD*)(source + 0x19) - *(WORD*)(source + 0x12);
	*noUnpackedBytes += (source[0x20] == 0xb8) ? 1 : *(WORD*)(source + 0x1c);

	dehrum((BYTE*)source + 0x91, unpackBuffer);

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
	if(data[5] != 0x21 ||
		data[8] != 0x11 ||
		data[11] != 0x01 ||
		data[12] != 0x77 ||
		data[13] != 0x00 ||
		data[14] != 0xd5 ||
		data[15] != 0xed) return false;

	WORD packed_size = *(WORD*)(data + 0x1c) + 5;
	WORD size        = *(WORD*)(data + 0x19) - *(WORD*)(data + 0x12);
	size            += (data[0x20] == 0xb8) ? 1 : *(WORD*)(data + 0x1c);

	if(packed_size > size || packed_size > file_size) return false;

	result.action = do_extract;
	strcpy(result.title, "[hrum 3.5i]");

	return true;
}
}
