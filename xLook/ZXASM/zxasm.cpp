#include <windows.h>
#include "../dtr.hpp"
#include <ctype.h>

namespace ZXASM
{

char token[][9]  =
{
"ld",    "ex",      "im",     "rst",     "ret",    "add",     "adc",     "sub",   "sbc",  "and",  "xor",   "or",    "cp",      "push",   "pop",     "inc",
"dec",   "in",      "out",    "jp",      "call",   "jr",      "djnz",    "rlc",   "rrc",  "rl",   "rr",    "sla",   "sra",     "sli",    "srl",     "bit",
"res",   "set",     "nop",    "halt",    "di",     "ei",      "rlca",    "rla",   "rrca", "rra",  "exx",   "daa",   "cpl",     "ccf",    "scf",     "ldi",
"ldir",  "ldd",     "lddr",   "cpi",     "cpir",   "cpd",     "cpdr",    "neg",   "inf",  "ini",  "inir",  "ind",   "indr",    "outi",   "otir",    "outd",
"otdr",  "reti",    "retn",   "rld",     "rrd",    "org",     "equ",     "db",    "dw",   "ds",   "defb",  "defw",  "defs",    "insert", "include", "if",
"ifdef", "ifndef",  "ifused", "ifnused", "else",   "endif",   "make",    "b",     "c",    "d",    "e",     "h",     "l",       "(hl)",   "a",       "xh",
"xl",    "yh",      "yl",     "(ix",     "(iy",    "(bc)",    "(de)",    "i",     "r",    "af",   "bc",    "de",    "hl",      "ix",     "iy",      "sp",
"(sp)",  "af'",     "(c)",    "nz",      "z",      "nc",      "c",       "po",    "pe",   "p",    "m",     "phase", "unphase", "dc",     "ent",     "repeat",
"endr",  "loadtab", "macro",  "endm",    "create", "makelab", "saveobj", "exitm", "ifp",  "exa",  "retz",  "retnz", "retc",    "retnc",  "retm",    "retp",
"retpo", "retpe",   "jpz",    "jpnz",    "jpc",    "jpnc",    "jpm",     "jpp",   "jppo", "jppe", "callz", "callc", "callm",   "callpe", "callnz",  "callnc",
"callp", "callpo",  "jrz",    "jrnz",    "jrc",    "jrnc"
};

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	WORD fromSize = hdr->length;
	if(!fromSize) return false;
	BYTE *from = (BYTE*)source;
	BYTE *to = unpackBuffer;

	while(fromSize--)
	{
		if(*from == 0x0D)
		{
			*to++ = '\r';
			*to++ = '\n';
			from++;
			continue;
		}

		if(*from<2 || *from>6 || fromSize<1)
		{
			*to++ = *from++;
			continue;
		}

		BYTE code=*(from+1);

		if(*from == 0x06)
		{
			code &= 0x7F;
			for(int j=0; j<code; j++)
				*to++ = 0x20;
		}
		else
		{
			if(code<0x20 || code>0xC5)
			{
				*to++ = *from++;
				continue;
			}
			else
			{
				if((*from)&1)
					for(char *p = token[code-0x20]; *p; ++p) *p = toupper(*p);
				else
					for(char *p = token[code-0x20]; *p; ++p) *p = tolower(*p);

				char *ptr = token[code - 0x20];
				while(*ptr)
					*to++ = *ptr++;

				if(!((*from)&2))
					*to++ = ' ';
			}
		}

		fromSize--;
		from+=2;
	} 

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr *hdr, const BYTE*, int)
{
	if(!hdr) return false;

	if(!(hdr->type == 'a' && hdr->type1 == 's' && hdr->type2 == 'm') &&
		!(hdr->type == 'a' && hdr->type1 == ' ' && hdr->type2 == ' ') &&
		!(hdr->type == 'z' && hdr->type1 == 'a' && hdr->type2 == 's') &&
		!(hdr->type == 'C' && (hdr->start == 35151)) &&
		!(hdr->type == 'a' && (hdr->start == 28001))) return false;

	result.action = do_extract;
	strcpy(result.title, "[zxasm]");
	strcpy(result.ext,   ".a80");

	return true;
}
}
