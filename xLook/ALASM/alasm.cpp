#include <windows.h>
#include "../dtr.hpp"

namespace ALASM
{

char mnemtkn[][8]  = { "INCLUDE", "INCBIN", "MACRO", "LOCAL", "RLCA", "RRCA", "HALT", "CALL", "PUSH", "RETN", "RETI", "DJNZ", "OUTI", "OUTD", "LDIR", "CPIR", "INIR", "OTIR", "LDDR", "CPDR", "INDR", "OTDR", "DD", "DEFB", "DEFW", "DEFS", "DISP", "ENDM", "EDUP", "ENDL", "MAIN", "ELSE", "DISPLAY", "EXA", "DB", "DW", "DS", "NOP", "INC", "DEC", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF", "ADD", "ADC", "SUB", "SBC", "AND", "XOR", "RET", "POP", "RST", "EXX", "RLC", "RRC", "SLA", "SRA", "SLI", "SRL", "BIT", "RES", "SET", "OUT", "NEG", "RRD", "RLD", "LDI", "CPI", "INI", "LDD", "CPD", "IND", "ORG", "EQU", "ENT", "INF", "DUP", "IFN", "REPEAT", "UNTIL" , "IF", "LD", "JR", "JP", "OR", "CP", "EX", "DI", "EI", "IN", "RL", "RR", "IM", "ENDIF", "EXD", "JNZ", "JZ", "JNC", "JC" };
char regstkn1[][8] = { "(BC)", "(DE)", "(HL)", "(SP)", "(IX)", "(IY)" };
char regstkn2[][8] = { "(C)" , "(IX" , "(IY" , "AF'" };
char regstkn3[][8] = { "BC", "DE", "HL", "AF", "IX", "IY", "SP", "NZ", "NC", "PO", "PE", "HX", "LX", "HY", "LY", "B" , "C" , "D" , "E" , "H" , "L" , "A" , "P" , "M" , "Z" , "R" , "I" };

bool WINAPI extract(HoHdr*, const BYTE *source, DWORD, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *p = (BYTE*)source + 9 + 0x18 + 2 + 2 + 1 + 1 + 1 + 8 + 16;
	BYTE *to = unpackBuffer;

	while(true)
	{
		BYTE len = *p++;
		if(len == 0) break;

		bool comment     = false;
		bool string      = false;
		bool russian     = false;
		bool tab_used    = false;
		bool first_token = true;

		size_t pos = 0;

		for(int j = 0; j < len-1 ; ++j)
		{
			BYTE b = *p++;
			if(b == 0xff) continue;

			if(comment || russian)
			{
				if(b < 0x20) continue; // skip tabs
				*to++ = b;
				++pos;
				continue;
			}
			if(string)
			{
				if(b == '"') string = false;
				if(b < 0x20) continue; // skip tabs
				*to++ = b;
				++pos;
				continue;
			}

			if(b == ';') comment = true;  // comment by EOL
			if(b == '"') string = true; // sting by '"'
			if(b == 0x10) // russian by EOL
			{
				russian = true;
				continue;
			}
			if(b < 0x10) // tabs
			{
				for(int k = 0; k < b; ++k) *to++ = ' ';
				pos += b;
				tab_used = true;
				continue;
			}

			if(b >= 0x80)
			{
				if(first_token)
				{
					first_token = false;
					if(pos < 8 && !tab_used)
						for(size_t k = 0; k < 8-pos; ++k) *to++ = ' ';
					pos += 8-pos;

					if(b <= 0xe5)
					{
						char *from = mnemtkn[b - 0x80];
						while(*from) *to++ = *from++;
						*to++ = '\t';
						//*to++ = ' ';
						pos += strlen(mnemtkn[b - 0x80]);
					}
				}
				else
				{
					if(b >= 0x9f && b <= 0xa4)
					{
						char *from = regstkn1[b - 0x9f];
						while(*from) *to++ = *from++;
						pos += strlen(regstkn1[b - 0x9f]);
					}
					if(b >= 0xd0 && b <= 0xd3)
					{
						char *from = regstkn2[b - 0xd0];
						while(*from) *to++ = *from++;
						pos += strlen(regstkn2[b - 0xd0]);
					}
					if(b >= 0xe0 && b <= 0xfa)
					{
						char *from = regstkn3[b - 0xe0];
						while(*from) *to++ = *from++;
						pos += strlen(regstkn3[b - 0xe0]);
					}
				}
				continue;
			}

			*to++ = b;
			++pos;
		}
		*to++ = '\r';
		*to++ = '\n';
	}

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int)
{
	const BYTE *p = data + 9 + 0x18 + 2 + 2 + 1 + 1 + 1;
	BYTE signature[8] = {0xF3,0x76,0xC7,0xDD,0xFD,0xED,0xB0,0xD9};

	for(int i = 0; i < sizeof(signature); ++i)
		if(*p++ != signature[i]) return false;

	result.action = do_extract;
	strcpy(result.title, "[alasm]");
	strcpy(result.ext,   ".a80");

	return true;
}
}
