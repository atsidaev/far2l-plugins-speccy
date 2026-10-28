#include <windows.h>
#include "../dtr.hpp"

namespace TASM
{

char token[][9]  =
{
"A",    "ADC ", "ADD ", "AF'",  "AF",   "AND ",     "B",       "BC",    "BIT ",  "C",     "CALL ", "CCF",    "CP ",   "CPD",  "CPDR", "CPI",
"CPIR", "CPL",  "D",    "DAA",  "DE",   "DEC ",     "DEFB ",   "DEFM ", "DEFS ", "DEFW ", "DI",    "PHASE ", "DJNZ ", "E",    "EI",   "UNPHASE ",
"EQU ", "EX ",  "EXX",  "H",    "HALT", "HL",       "I",       "IM ",   "IN ",   "INC ",  "IND",   "INDR",   "INI",   "INIR", "IX",   "IY",
"JP ",  "JR ",  "L",    "LD ",  "LDD",  "LDDR",     "LDI",     "LDIR",  "M",     "NC",    "NEG",   "NOP",    "NV",    "NZ",   "OR ",  "ORG ",
"OTDR", "OTIR", "OUT ", "OUTD", "OUTI", "P",        "PE",      "PO",    "POP ",  "PUSH ", "R",     "RES ",   "RET",   "RETI", "RETN", "RL ",
"RLA",  "RLC ", "RLCA", "RLD",  "RR ",  "RRA",      "RRC ",    "RRCA",  "RRD",   "RST ",  "SBC ",  "SCF",    "SET ",  "SLA ", "SP",   "SRA ",
"SRL ", "SUB ", "V",    "XOR ", "Z",    "INCLUDE ", "INCBIN ", "SLI ",  "INF",   "LX",    "HX",    "LY",     "HY",    "DB ",  "DM ",  "DS ",
"DW ",  "?",    "?",    "?",    "?",    "?",        "?",       "?",     "?",     "?",     "?",     "?",      "?",     "?",    "?",    "?"
};

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	bool isTasm4 = hdr->start <= 4096;
	
	BYTE *from = (BYTE*)source;
	BYTE *to = unpackBuffer;

	while(true)
	{
		BYTE len = *from++;

		if(len == 0xFF)
			break;

		for(BYTE i=0; i<len; i++)
		{
			if(*from < 0x20)
			{
				if(isTasm4 || ((*from == 0x0A) && (i!=(len-1))))
				{
					if(!isTasm4 || (isTasm4 && (*from == 0x01)))
					{
						from++;
						i++;
					}
					for(int j=0; j<*from; j++)
						*to++ = 0x20;
				}
				else
					*to++ = *from;
			}
			else if(*from < 0x80)
				*to++ = *from;
			else
			{
				char *ptr;
				if(!isTasm4)
					ptr = token[*from - 0x80];
				else
					switch(*from)
					{
					case 0x97://"DEFM "
						ptr = "DEFMAC ";
						break;
					case 0x9B://"PHASE "
						ptr = "DISPLAY ";
						break;
					case 0x9F://"UNPHASE "
						ptr = "ENDMAC ";
						break;
					default:
						ptr = token[*from - 0x80];
					}
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

	if(!(hdr->type == 'A' && (hdr->start == 39221)) &&
		!(hdr->type == 'A' && (hdr->start == 40872)) &&
		!(hdr->type == 'A' && (hdr->start <= 4096))) return false;

	result.action = do_extract;
	strcpy(result.title, "[tasm]");
	strcpy(result.ext,   ".a80");

	return true;
}
}
