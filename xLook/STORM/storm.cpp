#include <windows.h>
#include "../dtr.hpp"

namespace STORM
{

char tokens[][5] =
{
/*0x09 */ "ORG",  "EQU",  "DI",   "EI",   "EXA",  "NOP",  "CCF",  "SCF",
/*0x11 */ "CPL",  "DAA",  "EXX",  "RLA",  "RRA",  "RLCA", "RRCA", "HALT",
/*0x19 */ "LDI",  "LDD",  "LDIR", "LDDR", "CPI",  "CPD",  "CPIR", "CPDR",
/*0x21 */ "INI",  "IND",  "INIR", "INDR", "OUTI", "OUTD", "OTIR", "OTDR",
/*0x29 */ "NEG",  "RLD",  "RRD",  "INF",  "RETI", "RETN", "",     "B",
/*0x31 */ "C",    "D",    "E",    "H",    "L",    "(HL)", "A",    "HX",
/*0x39 */ "LX",   "HY",   "LY",   "BC",   "DE",   "HL",   "SP",   "",
/*0x41 */ "DEFB", "DEFW", "DEFS", "AF'",  "XH",   "XL",   "YH",   "YL",
/*0x49 */ "",     "LD",   "INC",  "DEC",  "EX",   "JR",   "DJNZ", "JP",
/*0x51 */ "CALL", "RET",  "POP",  "PUSH", "ADD",  "ADC",  "SUB",  "SBC",
/*0x59 */ "AND",  "OR",   "XOR",  "CP",   "IN",   "OUT",  "BIT",  "RES",
/*0x61 */ "SET",  "RLC",  "RRC",  "RL",   "RR",   "SLA",  "SRA",  "SLI",
/*0x69 */ "SRL",  "IM",   "RST",  "DB",   "DW",   "DS",   "IX",   "IY",
/*0x71 */ "(BC)", "(DE)", "I",    "R",    "AF",   "(SP)", "(C)",  "NZ",
/*0x79 */ "Z",    "NC",   "C",    "PO",   "PE",   "P",    "M",    "INCL",
/*0x81 */ "INCB", "REPT", "ENDR", "IF",   "IFU",  "IFNU", "IFD",  "IFND",
/*0x89 */ "ELSE", "EIF",  "ENDM"
};

char ar[][3] =
{
	"+", "-", "*", "/", "\\", "&", "!", "|",
	"<<",">>","<=",">=","<",  ">", "=",
	"[", "]", "^", "`", "?", "~",  "@", "'"
};

struct LineBuffer
{
	char  buf[256];
	char* p;

	void clearLine() {p = buf;}

	void putChar  (char  c) {*p++ = c;}
	void putString(char* s) {while(*s) *p++ = *s++;}
	void putInt   (int   n)
	{
		char tmp[6];
		sprintf(tmp, "%d", n);
		putString(tmp);
	}

	void putHex(BYTE b)
	{
		BYTE high = b>>4 & 0x0f;
		putChar("0123456789ABCDEF"[high]);
		BYTE low  = b & 0x0f;
		putChar("0123456789ABCDEF"[low]);
	}

	void putBin(BYTE b)
	{
		BYTE mask = 0x80;
		for(int i = 0; i < 8; ++i)
		{
			(b & mask) ? putChar('1') : putChar('0');
			mask >>= 1;
		}
	}

	void fillTo(int pos)
	{
		int curPos = p - buf;
		for(int i = curPos; i < pos; ++i) putChar(' ');
	}
};

LineBuffer lb;

bool bit(BYTE num, BYTE b)
{
	BYTE mask = 1 << num;
	return ((b & mask) != 0);
}

struct Line
{
	BYTE  length;
	BYTE* start;
	Line* next;
	//Line(BYTE* _start, BYTE _length, Line* _next) : start(_start), length(_length), next(_next) {}
};

struct Lines
{
	Line* top;
	Line* cur;
	void clearLines(void)
	{
		Line* l = top;
		while(l)
		{
			Line* tmp = l;
			l = l->next;
			free(tmp);
		}
	}
	void push(BYTE* _start, BYTE _length)
	{
		Line* newLine = (Line*)malloc(sizeof(Line));
		newLine->start = _start;
		newLine->length = _length;
		newLine->next = top;
		top = cur = newLine;
	}
	Line* pop(void)
	{
		if(!cur) return 0;
		Line* ret = cur;
		cur = cur->next;
		return ret;
	}
};

BYTE* printString(BYTE* p)
{
	while(*p)
	{
		if(*p <= 0x1f)
			for(BYTE i = 0; i < *p; ++i) lb.putChar(' ');
		else
			lb.putChar(*p);
		++p;
	}
	return ++p;
}

BYTE* printLabel(BYTE* p)
{
	BYTE b = *p;
	if(b == 0xDA)
		lb.putChar('_');
	else if(b > 0xDA) // ???
		lb.putChar('=');
	else
		lb.putChar(b-0x7f);

	do
	{
		++p;
		BYTE b = *p & 0x3f;

		if(b <= 0x09)
			lb.putChar('0' + b);
		else if(b == 0x0b)
			lb.putChar('_');
		else if(b == 0x0a) {/* 0x8a|0x0a - end of one char label */}
		else if(b < 0x26)
			lb.putChar(b - 0x0c + 'A');
		else
			lb.putChar(b - 0x26 + 'a');
	} while(*p < 0x80);
	return p+1;
}

BYTE* printCommand(BYTE* p, bool indent)
{
	if(indent) lb.fillTo(8);

	BYTE b = *p;
	if(b >= 0x9 && b < 0x2f)
	{
		lb.putString(tokens[b-9]);
		++p;
	}
	else if(b > 0x2f && b < 0x40)
	{
		lb.putString("LD");
	}
	else if(b >= 0x40 && b < 0x6f)
	{
		if(b == 0x49)
		{
			lb.putString(tokens[p[1]+0x80-0x09]);
			p += 2;
		}
		else
		{
			lb.putString(tokens[b-9]);
			++p;
		}
	}
	else if(b >= 0x6f && b < 0x75)
	{
		lb.putString("LD");
	}
	else if(b >= 0x75 && b < 0x77)
	{
		lb.putString("EX");
	}
	else if(b == 0x77)
	{
		lb.putString("OUT");
	}
	else if(b > 0x77 && b < 0x7c)
	{
		lb.putString("JR");
	}
	else if(b >= 0x7c && b < 0x80)
	{
		lb.putString("CALL");
	}
	else if(b >= 0x80 && b < 0xDC)
	{
		bit(5,b) ? lb.putString("LD") : lb.putString("JR");
	}
	else if(b == 0xDC)
	{
		lb.putString("DB");
	}
	else if(b > 0xDC)
	{
		lb.putString("JR");
	}
	return p;
}

BYTE* printExpression(BYTE* p, BYTE op, bool number)
{
	while(true)
	{
		if(number)
		{
			switch(op & 0x07)
			{
			case 0:
				{
					lb.putChar('(');
					BYTE b = *p++;
					p = printExpression(p, b, !bit(4,op));
					lb.putChar(')');
				}
				break;
			case 1:
				lb.putChar('#');
				lb.putHex (*p++);
				break;
			case 2:
				lb.putInt(*p++);
				break;
			case 3:
				if(*p < 0xC0)
				{
					BYTE b = *p++;
					if(b == 0)
						lb.putChar('$');
					else
					{
						--b;
						b &= 0x07;
						lb.putChar('='); // ???
						lb.putChar('0' + b);
					}
				}
				else
					p = printLabel(p);
				break;
			case 4:
				lb.putChar('"');
				if(p[1] != 0) lb.putChar(p[1]);
				lb.putChar(*p);
				p += 2;
				lb.putChar('"');
				break;
			case 5:
				lb.putChar('#');
				lb.putHex (p[1]);
				lb.putHex (p[0]);
				p += 2;
				break;
			case 6:
				lb.putInt(256*p[1] + p[0]);
				p += 2;
				break;
			case 7:
				lb.putChar('%');
				if(p[1] != 0) lb.putBin(p[1]);
				lb.putBin(*p);
				p+= 2;
				break;
			}

			number = false;
			if(bit(3,op)) break;
			op = *p++;
		}
		else
		{
			BYTE _b = op;
			if((op & 0xf0) != 0xf0)
			{
				op &= 0xf0;
				op >>= 4;
				lb.putString(ar[op]);
				op = _b;
				number = true;
			}
			else
			{
				op &= 0x07;
				op += 0x0f;

				lb.putString(ar[op]);
				number = false;

				// if it is last postfix operator...
				if(bit(3,_b)) break;
				op = *p++;
			}
		}
	}
	return p;
}

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	memset(unpackBuffer, 0, sizeof(noUnpackedBytes));

	Lines ls;
	ls.top = ls.cur = 0;

	BYTE *p = (BYTE*)source + hdr->length - 1;

	// fill list of lines
	while(p - source > 0)
	{
		BYTE length = *p & 0x3f;
		ls.push(p-length, length);
		p -= length+1;
	}
	Line* l = 0;

	char separator;
	bool indent;
	while(l = ls.pop())
	{
		lb.clearLine();
		BYTE len = l->length;
		BYTE*  p = l->start;

		if(!len) goto EOL;
		if(*p == 0x2F)
		{
			lb.putChar(';');
			printString(p+1);
			goto EOL;
		}

		indent = true;

		if(*p >= 0xC0 && *p < 0xDC)
		{
			if(bit(6,p[1])) p = printLabel(p);
		}
		else if(*p == 0x06)
		{
			lb.putChar('_');
			++p;
			indent = false;
		}
		else if(*p == 0x07)
		{
			lb.putChar('.');
			lb.putInt(p[1]);
			p += 2;
		}
		if(len-(p-l->start) <= 0) goto EOL;

COMMAND:
		p = printCommand(p, indent);
		indent = false;

		separator = ' ';
		while(true)
		{
			if(len-(p-l->start) <= 0) break;

			if(*p == 0x2F)
			{
				lb.putString(";");
				p = printString(p+1);
				break;
			}
			if(*p < 0x80)
			{
				BYTE b = *p & 0x3f;
				if(b >= 0x2a && b < 0x2f)
				{
					switch(b)
					{
					case 0x2a:
						lb.putString(":");
						break;
					case 0x2b:
						lb.putString(" :");
						break;
					case 0x2c:
						lb.putString(" : ");
						break;
					case 0x2d:
						lb.putString(" :  ");
						break;
					case 0x2e:
						lb.putString("  : ");
						break;
					}
					++p;
					goto COMMAND;
				}
			}

			lb.putChar(separator);
			separator = ',';

			if(*p < 0x80) // token
			{
				lb.putString(tokens[*p-9]);
				++p;
			}
			else if(*p >= 0x80 && *p < 0xC0) // expression
			{
				BYTE b = *p++;
				bool needBrackets = bit(5,b);
				if(needBrackets) lb.putChar('(');

				bool number = true;
				if(!needBrackets)
				{
					number = !bit(4,b);
					if(!number) b &= 0x1f;
				}

				if(needBrackets && bit(4,b)) // IX/IY operations
				{
					lb.putChar('I');
					bit(3,b) ? lb.putChar('Y') : lb.putChar('X');
					b = b & 0x07;
					if(!b)
					{
						lb.putChar(')');
						continue;
					}
					else
					{
						if((b&0x03) == 0)
							b = *p++;
						else
						{
							if(bit(2,b))
							{
								b &= 0x03; // operation + or -
								b |= 0x10;
							}
							b |= 0x08; // one number required
						}
						number = false;
					}
				}
				else
					if(!number) b &= 0x1f;

				p = printExpression(p, b, number);

				if(needBrackets) lb.putChar(')');
			}
			else if(*p >= 0xC0 && *p <= 0xDB) // label
			{
				p = printLabel(p);
			}
			else if(*p == 0xDC) // quoted srting
			{
				lb.putChar('"');
				p = printString(p+1);
				lb.putChar('"');
			}
			else if(*p > 0xDC && *p <= 0xE6) // one digit
			{
				lb.putChar(*p - 0xAD);
				++p;
			}
			else // > 0xE6 - $[+-]offset
			{
				lb.putChar('$');
				if(*p < 0xf3)
				{
					lb.putChar('-');
					lb.putInt (0xF3 - *p);
				}
				else if(*p > 0xf3)
				{
					lb.putChar('+');
					lb.putInt (*p - 0xF3);
				}
				++p;
			}
		}

		// this part of code will not be executed
		/*BYTE rest = len - (p - l->start);
		lb.putHex(len);
		lb.putString(": ");
		for(BYTE i = 0; i < rest; ++i)
		{
			if(i > 0) lb.putString(", ");
			lb.putHex(p[i]);
		}*/

EOL:
		*lb.p = 0;
		strcat((char*)lb.buf, "\r\n");
		strcat((char*)unpackBuffer, (char*)lb.buf);
	}
	ls.clearLines();

	*noUnpackedBytes = strlen((char*)unpackBuffer);

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr *hdr, const BYTE*, int)
{
	if(!hdr) return false;

	if(!(hdr->type == 'C' && (hdr->start == 0xC00B)) &&
		!(hdr->type == 'C' && (hdr->start == 0xC003)) &&
		!(hdr->type == 'R' && (hdr->start == 0xC00B))) return false;

	result.action = do_extract;
	strcpy(result.title, "[storm]");
	strcpy(result.ext,   ".a80");

	return true;
}
}
