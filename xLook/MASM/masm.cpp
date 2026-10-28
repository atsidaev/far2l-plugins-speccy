#include <windows.h>
#include "../dtr.hpp"

namespace MASM
{

char token[][9]  =
{
"A",      "B" ,      "C",    "D",     "E",    "H",      "L",       "I",    "R",    "XH",     "XL",   "YH",      "YL",       "IX",   "IY",    "AF'",
"AF",     "HL",      "DE",   "BC",    "M",    "NC",     "NV",      "NZ",   "P",    "PE",     "PO",   "V",       "Z",        "SP",   "{ ",    "ORG ",
"PHASE ", "UNPHASE", "AND ", "ADC ",  "SBC ", "ADD ",   "SUB ",    "XOR ", "OR ",  "CP ",    "LD ",  "IM ",     "RST ",     "EI",   "DI",    "EXX",
"EXA",    "INF",     "LDIR", "LDDR",  "OTIR", "OTDR",   "OUTI",    "OUTD", "RETI", "RETN",   "INIR", "INDR",    "CPIR",     "CPDR", "NEG",   "CPD",
"CPI",    "IND",     "INI",  "LDD",   "LDI",  "CCF",    "CPL",     "DAA",  "HALT", "NOP",    "RLA",  "RLCA",    "RRA",      "RRCA", "SCF",   "RLD",
"RRD",    "EX ",     "RET",  "CALL ", "JP ",  "PUSH ",  "POP ",    "INC ", "DEC ", "OUT ",   "IN ",  "DJNZ ",   "JR ",      "BIT ", "RLC ",  "RRC ",
"RL ",    "RR ",     "SLA ", "SRA ",  "SLI ", "SRL ",   "RES ",    "SET ", "EQU ", "BEGIN ", "END",  "INCBIN ", "INCLUDE ", "DB ",  "DEFB ", "DEFS ",
"DEFW ",  "DS ",     "DW ",  "DOWN",  "UP",   "SYSTEM", "STOPKEY", "?",    "?",    "?",      "?",    "?",       "?",        "?",    "?",     "?"
};

bool WINAPI extract(HoHdr*, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *from = (BYTE*)source;
	BYTE *to = unpackBuffer;

	while(true)
	{
		BYTE len = *from++;

		if(len == 0xFF)
			break;

		if(!len)
		{
			*to++ = '\r';
			*to++ = '\n';
			continue;
		}

		for(BYTE i=0; i<len; i++)
		{
			if(*from < 0x20)
			{
				if((*from == 0x0A) && (i!=(len-1)))
				{
					from++;
					i++;
					for(int j=0; j<*from; j++)
						*to++ = ' ';
				}
				else
					*to++ = *from;
			}
			else if(*from < 0x80)
				*to++ = *from;
			else
			{
				char *ptr = token[*from - 0x80];
				while(*ptr)
					*to++ = *ptr++;
			}
			from++;
			if(from - source > sourceSize)
				break;
		}

		*to++ = '\r';
		*to++ = '\n';
		if(*from != len)
			break;
		from++;
		if(from - source > sourceSize)
			break;
	}

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr *hdr, const BYTE*, int)
{
	if(!hdr) return false;

	if(!(hdr->type == 'a' && (hdr->start == 38667)) &&
		!(hdr->type == 'a' && (hdr->start == 38821))) return false;

	result.action = do_extract;
	strcpy(result.title, "[masm]");
	strcpy(result.ext,   ".a80");

	return true;
}
}
