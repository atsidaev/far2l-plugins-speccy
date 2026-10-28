#include <windows.h>
#include "../dtr.hpp"

namespace XAS
{

char opcodes[][10] =
{
"LDIR", "LDDR", "LDI",  "LDD",  "CPIR", "CPDR", "CPI", "CPD",  "INIR", "INDR", "INI",  "IND",  "OUTI",  "OTIR",  "OUTD",  "OTDR",
"RETI", "RETN", "NEG",  "RLD",  "RRD",  "PUSH", "POP", "ADD",  "SUB",  "ADC",  "SBC",  "AND",  "OR",    "XOR",   "CP",    "INC",
"DEC",  "BIT",  "RES",  "SET",  "RRC",  "RLC",  "RL",  "RR",   "SLA",  "SRA",  "SLI",  "SRL",  "LD",    "EX",    "IN",    "OUT",
"IM",   "RST",  "DJNZ", "JP",   "JR",   "CALL", "RET", "EXX",  "CPL",  "DAA",  "RLCA", "RRCA", "RLA",   "RRA",   "NOP",   "HALT",
"DI",   "EI",   "SCF",  "CCF",  "ORG",  "ENT",  "EQU", "WORK", "DB",   "DW",   "DM",   "DS",   "!ASSM", "!CONT", "LTEXT", "LCODE",
"BC",   "DE",   "HL",   "IX",   "IY",   "SP",   "AF",  "(C)",  "B",    "C",    "D",    "E",    "H",     "L",     "(HL)",  "A",
"(BC)", "(DE)", "HX",   "LX",   "HY",   "LY",   "I",   "R",    "NZ",   "Z",    "NC",   "PO",   "PE",    "P",     "M",     "!ON",
"!OFF", "(SP)", "AF'",  "USEL", "IFNZ", "IFZ",  "MAKE" "?",    "?",    "?",    "?",    "?",    "?",     "?",     "?",     "?"
};

BYTE cyr(byte b)
{
	// OEM code page
	byte cyr1[] = {0x84, 0x86, 0x88, 0x89, 0x8b, 0x8f, 0x93, 0x94, 0x96, 0x97, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0x9a};
	byte cyr2[] = {0x98, 0x99, 0x81, 0x83};

	if(b >= 0x10 && b <= 0x1f) return cyr1[b-0x10];
	if(b >= 0x7b && b <= 0x7e) return cyr2[b-0x7b];
	return b;
}

BYTE toLower(byte b)
{
	// convert all UPPERCASE letters to lowercase
	if(b >= 'A' && b <= 'Z') return b - 'A' + 'a';
	return b;
}

BYTE transform(byte b)
{
	b = cyr    (b);
	b = toLower(b);
	return b;
}

bool WINAPI extract(HoHdr*, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *p = (BYTE*)source + 29 + 2 + 1 + 1 + 1 + 1 + 1;
	BYTE *to = unpackBuffer;
	bool needComma;
	char *from;

	while(*p)
	{
		byte b = *p++;
		// process the label
		while(b < 0x80)
		{
			if(b == 0x0d || b == 0x0c || b == 0x09)
			{
				// EOL
				*to++ = '\r';
				*to++ = '\n';
				goto NEXT_LINE;
			}
			if(b == ';')
			{
				// it's comment;
				while(b != 0x0d && b != 0x0c && b != 0x09)
				{
					*to++ = cyr(b);
					b = *p++;
				}
				*to++ = '\r';
				*to++ = '\n';
				goto NEXT_LINE;
			}
			*to++ = toLower(b);
			b = *p++;
		}

		// command
		*to++ = '\t';
		*to++ = '\t';
		from = opcodes[b-0x80];
		while(*from) *to++ = *from++;
		*to++ = '\t';

		needComma = false;

		// loop foreach token
		while(true)
		{
			b = *p++;
			if(b == 0x0d || b == 0x0c || b == 0x09)
			{
				// EOL
				*to++ = '\r';
				*to++ = '\n';
				goto NEXT_LINE;
			}
			if(b == ';')
			{
				*to++ = '\t';
				while(b != 0x0d && b != 0x0c && b != 0x09)
				{
					*to++ = cyr(b);
					b = *p++;
				}
				*to++ = '\r';
				*to++ = '\n';
				goto NEXT_LINE;
			}
			if(b >= 0x80)
			{
				if(needComma) *to++ = ',';

				char *from = opcodes[b-0x80];
				while(*from) *to++ = *from++;
				needComma = true;
				continue;
			}
			if(b == '(')
			{
				if(needComma) *to++ = ',';
				*to++ = '(';

				b = *p++;
				while(b != ')')
				{
					if(b >= 0x80)
					{
						char *from = opcodes[b-0x80];
						while(*from) *to++ = *from++;
					}
					else
						*to++ = toLower(b);
					b = *p++;
				}
				*to++ = ')';
				needComma = true;
				continue;
			}
			if(b == '"')
			{
				if(needComma) *to++ = ',';
				*to++ = '"';

				// special case """
				if(p[0] == '"' && p[1] == '"')
				{
					p += 2;
					*to++ = '"';
					*to++ = '"';
					continue;
				}
				b = *p++;
				while(b != '"')
				{
					*to++ = cyr(b);
					b = *p++;
				}
				*to++ = '"';
				needComma = true;
				continue;
			}

			if(needComma) *to++ = ',';
			while(b < 0x80 && b != '(' && b != ';' && b != 0x0d && b != 0x0c && b != 0x09)
			{
				*to++ = toLower(b);
				b = *p++;
			}
			needComma = true;
			--p;
		}

		*to++ = '\r';
		*to++ = '\n';
NEXT_LINE:;
	}

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr *hdr, const BYTE*, int)
{
	if(!hdr) return false;

	if((hdr->type != 'X' && hdr->type != 'x') ||
		(hdr->type1 != 'A' && hdr->type1 != 'a') ||
		hdr->type2 != 'S') return false;

	result.action = do_extract;
	strcpy(result.title, "[xas]");
	strcpy(result.ext,   ".a80");

	return true;
}
}
