#include <windows.h>
#include "../dtr.hpp"
#include "widestring.hpp"

namespace BASIC
{

bool showNumbers   = true;
bool showCtrlChars = false;
bool stripComments = true;

DWORD _file_size = 0;
BYTE *_data = 0;
BYTE *_p = 0;

bool get_line(char* buf, int max_size = 100)
{
	if(_p - _data >= _file_size) return false;  // EOF

	char *to = buf;
	while(true)
	{
		if(_p - _data >= _file_size) break; // EOF
		if(_p[0] == 0x0d && _p[1] == 0x0a) { _p += 2; break; }  // EOL
		if(_p[0] == 0x0a) { ++_p; break; }  // EOL
		*to++ = *_p++;

		if(to - buf == max_size - 1) break; // buffer full
	}

	*to = 0;
	return true;
}

char* skip_white_spaces(char* p)
{
	while(*p && (*p == ' ' || *p == '\t')) ++p;
	return p;
}

bool read_ini(const char* path)
{
	char full_path[300];
	strcpy(full_path, path);
	strcat(full_path, "basic.ini");

	HANDLE file = CreateFile(_W(full_path).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	// file not found
	if(file == INVALID_HANDLE_VALUE) return false;

	_file_size = SetFilePointer(file, 0, 0, FILE_END);
	SetFilePointer(file, 0, 0, FILE_BEGIN);

	_data = (BYTE*)malloc(_file_size + 1);
	DWORD read;
	ReadFile(file, _data, _file_size, &read, 0);
	CloseHandle(file);

	_p = _data;

	char line[100];

	while(get_line(line, sizeof(line)))
	{
		char *p = skip_white_spaces(line);
		// skip empty lines, comments and section titles
		if(*p == 0 || *p == ';' || *p == '#' || *p == '[') continue;

		char option[100];
		char *o = option;
		while(*p && *p != ' ' && *p != '\t') *o++ = *p++;
		*o = 0;

		p = skip_white_spaces(p);
		if(*p != '=') continue;
		++p;
		p = skip_white_spaces(p);

		char value[100];
		char *v = value;
		while(*p && *p != ' ' && *p != '\t' && *p != '#' && *p != ';') *v++ = *p++;
		*v = 0;
		
		if(strcasecmp(option,"showNumbers") == 0)
		{
			if(strcasecmp(value,"yes") == 0 || strcasecmp(value,"true") == 0 || strcasecmp(value,"yep") == 0 || strcasecmp(value,"1") == 0)
				showNumbers = true;
			else
				showNumbers = false;
		}
		if(strcasecmp(option,"showCtrlChars") == 0)
		{
			if(strcasecmp(value,"yes") == 0 || strcasecmp(value,"true") == 0 || strcasecmp(value,"yep") == 0 || strcasecmp(value,"1") == 0)
				showCtrlChars = true;
			else
				showCtrlChars = false;
		}
		if(strcasecmp(option,"stripComments") == 0)
		{
			if(strcasecmp(value,"yes") == 0 || strcasecmp(value,"true") == 0 || strcasecmp(value,"yep") == 0 || strcasecmp(value,"1") == 0)
				stripComments = true;
			else
				stripComments = false;
		}
	}
	free(_data);
	return true;
}

/*
 *  type = 1 - token can be first token in a line
 *  type = 0 - token can't be first token in a line
 */
struct Token
{
	char name[11];
	BYTE type;
};

Token tokenTable[] =
{
	{"RND ", 0 },   {"INKEY$", 0},   {"PI", 0},      {"FN ", 0},     {"POINT ", 1},
	{"SCREEN$", 0}, {"ATTR ", 0},    {"AT ", 0},     {"TAB", 0},     {"VAL$ ", 0},
	{" CODE ", 0},  {"VAL ", 0},     {"LEN ", 0},    {"SIN ", 0},    {"COS ", 0},
	{"TAN ", 0},    {"ASN ", 0},     {"ACS ", 0},    {"ATN ", 0},    {"LN ", 0},
	{"EXP ", 0},    {"INT ", 0},     {"SQR ", 0},    {"SGN ", 0},    {"ABS ", 0},
	{"PEEK ", 0},   {"IN ", 0},      {"USR ", 0},    {"STR$ ", 0},   {"CHR$ ", 0},
	{"NOT ", 0},    {"BIN ", 0},     {" OR ", 0},    {" AND ", 0},   {"<=", 0},
	{">=", 0},      {"<>", 0},       {"LINE ", 0},   {" THEN ", 0},  {" TO ", 0},
	{" STEP ", 0},  {"DEF FN ", 1},  {"CAT", 1},     {"FORMAT ", 1}, {"MOVE ", 1},
	{"ERASE ", 1},  {"OPEN# ", 1},   {"CLOSE# ", 1}, {"MERGE ", 1},  {"VERIFY ", 1},
	{"BEEP ", 1},   {"CIRCLE ", 1},  {"INK ", 2},    {"PAPER ", 2},  {"FLASH ", 2},
	{"BRIGHT ", 2}, {"INVERSE ", 2}, {"OVER ", 2},   {"OUT ", 1},    {"LPRINT ", 1},
	{"LLIST ", 1},  {"STOP", 1},     {"READ ", 1},   {"DATA ", 1},   {"RESTORE ", 1},
	{"NEW", 1},     {"BORDER ", 1},  {"CONTINUE", 1},{"DIM ", 1},    {"REM ", 1},
	{"FOR ", 1},    {"GO TO ", 1},   {"GO SUB ", 1}, {"INPUT ", 1},  {"LOAD ", 1},
	{"LIST ", 1},   {"LET ", 1},     {"PAUSE ", 1},  {"NEXT ", 1},   {"POKE ", 1},
	{"PRINT ", 1},  {"PLOT ", 1},    {"RUN ", 1},    {"SAVE ", 1},   {"RANDOMIZE ", 1},
	{"IF ", 1},     {"CLS", 1},      {"DRAW ", 1},   {"CLEAR ", 1},  {"RETURN", 1},
	{"COPY ", 1}
};

char CtrlCharTable1[][4] = {"AT", "TAB"};
char CtrlCharTable2[][8] = {"INK", "PAPER", "FLASH", "BRIGHT", "INVERSE", "OVER"};

int isTRDOSCommand(BYTE b)
{
	if(b > 206 && b < 215) return 1;
	switch(b)
	{
	case '*':
	case '4':
	case '8':
	case 190:
	case 230:
	case 236:
	case 239:
	case 240:
	case 244:
	case 247:
	case 248:
	case 254:
	case 255:
		return 1;
	}
	return 0;
}

enum {MCHAR, MBYTE, TOKEN, CTRL, CTRL1, CTRL2, NUMBER};

int getType(BYTE b)
{
	if(b > 164)  return TOKEN;
	if(b > 0x7f) return MBYTE;
	if(b > 0x1f) return MCHAR;
	if(b > 23)   return MBYTE;
	if(b > 21)   return CTRL2;
	if(b > 15)   return CTRL1;
	if(b == 14)  return NUMBER;
	if(b == 8 || b == 6)   return CTRL;
	return MBYTE;
}

void WINAPI init(const char* path)
{
	read_ini(path);
}

bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD)
{
	BYTE *to = unpackBuffer;
	char tmp[20];

	WORD basicSize = hdr->length;

	// some protection scheme use basicSize = 0
	if(basicSize < 6) basicSize = hdr->start;

	if(basicSize > sourceSize) basicSize = sourceSize;

	enum {NORMAL, INSIDE_QUOTAS, INSIDE_REM};

	BYTE *p = (BYTE*)source;
	while(p-(BYTE*)source < basicSize)
	{
		WORD lineNumber = 256*p[0] + p[1];
		p += 2;
		sprintf(tmp, "%5u ", lineNumber);

		char *from = tmp;
		while(*from) *to++ = *from++;

		WORD len = 256*p[1] + p[0];
		p += 2;

		if(p-(BYTE*)source >= basicSize) break;

		if(len < 2 || len > basicSize-(p-(BYTE*)source)) len = basicSize - (p-(BYTE*)source);

		int mode = NORMAL;
		int tokenDetected = 0;
		int tokType = 1;

		BYTE cb = 0, pb;
		WORD pos = 0;

		BYTE *line = p;
		while(p-line < len)
		{
			if(mode == INSIDE_REM && stripComments)
			{
				from = "[skiped]";
				while(*from) *to++ = *from++;
				p = line+len;
				break;
			}
		pb = cb;
		cb = *p++;

		BYTE arg1, arg2;
		switch(getType(cb))
		{
		case MBYTE:
			if(showCtrlChars)
			{
				sprintf(tmp, "[%u]", cb);
				from = tmp;
				while(*from) *to++ = *from++;
			}
			break;
		case CTRL:
			if(showCtrlChars)
			{
				if(cb == 6)
				{
					from = "[,]";
					while(*from) *to++ = *from++;
				}
				else
				{
					from = "[BS]";
					while(*from) *to++ = *from++;
				}
			}
			break;
		case CTRL1:
			arg1 = *p++;
			if(showCtrlChars)
			{
				sprintf(tmp, "[%s %u]", CtrlCharTable2[cb-16], arg1);
				from = tmp;
				while(*from) *to++ = *from++;
			}
			break;
		case CTRL2:
			arg1 = *p++;
			arg2 = *p++;
			if(showCtrlChars)
			{
				sprintf(tmp, "[%s  %u, %u]", CtrlCharTable1[cb-22], arg1, arg2);
				from = tmp;
				while(*from) *to++ = *from++;
			}
			break;
		case MCHAR:
			*to++ = cb;
			if(cb == ':' && mode == NORMAL) { tokType = 1; tokenDetected = 0; }
			if(cb == '"')
			{
				switch(mode)
				{
				case INSIDE_QUOTAS:
					mode = NORMAL;
					break;
				case NORMAL:
					mode = INSIDE_QUOTAS;
					break;
				}
			}
			break;
		case TOKEN:
			switch(mode)
			{
			case INSIDE_REM:
				{
					sprintf(tmp, "[%u]", cb);
					from = tmp;
					while(*from) *to++ = *from++;
				}
				break;
			case NORMAL:
				tokenDetected = 1;
				if(tokenTable[cb-165].type != 2 && tokType != tokenTable[cb-165].type) mode = INSIDE_REM;
				// if token THEN then next token may have type 1
				tokType = (cb == 203) ? 1 : 0;
				// if not REM and not TR-DOS command
				if(cb == 234 && (p[0] != ':' || !isTRDOSCommand(p[1]))) mode = INSIDE_REM;
			case INSIDE_QUOTAS:
				{
					from = tokenTable[cb-165].name;
					while(*from) *to++ = *from++;
				}
				break;
			}
			break;
			case NUMBER:
				if(mode == NORMAL && tokenDetected && (pb < 165 || pb == 196))
				{
					BYTE power = *p++;
					if(!power)
					{
						WORD number = 256*p[2] + p[1];
						p += 4;
						if(showNumbers)
						{
							sprintf(tmp, "{%u}", number);
							from = tmp;
							while(*from) *to++ = *from++;
						}
					}
					else
					{
						double floatNum = 1;
						int abs = power-160;
						if(abs < 0) abs = -abs;
						for(int i = 0; i < abs; i++) floatNum *= 2;
						if(power < 160) floatNum = 1/floatNum;

						union
						{
							WORD num;
							BYTE  u8num[4];
						} mantisa;

						mantisa.u8num[3] = *p++; mantisa.u8num[2] = *p++;
						mantisa.u8num[1] = *p++; mantisa.u8num[0] = *p++;

						if(mantisa.u8num[3] < 0x80)
							floatNum *= (double(mantisa.num)+double(0x80000000ul));
						else
							floatNum *= -double(mantisa.num);

						if(showNumbers)
						{
							*to++ = '{';
							// emulation of sprintf("%.2f", ...)
							if(floatNum < 0)
							{
								*to++ = '-';
								floatNum = -floatNum;
							}
							sprintf(tmp, "%d", (long)floatNum);
							from = tmp;
							while(*from) *to++ = *from++;
							floatNum -= (long)floatNum;
							floatNum *= 100; // 2 digits precision
							sprintf(tmp, ".%d}", (long)floatNum);
							from = tmp;
							while(*from) *to++ = *from++;
						}
					}
				}
				else
				{
					if(showCtrlChars)
					{
						sprintf(tmp, "[%u]", cb);
						from = tmp;
						while(*from) *to++ = *from++;
					}
				}
				break;
			}
		}
		*to++ = '\r';
		*to++ = '\n';
	}

	*noUnpackedBytes = to-unpackBuffer;

	return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr *hdr, const BYTE*, int)
{
	if(!hdr) return false;

	if(hdr->type != 'B')
		return false;

	result.action = do_extract;
	strcpy(result.title, "[zx basic]");
	strcpy(result.ext,   ".bas");
	return true;

}
}
