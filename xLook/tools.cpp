#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;
#include "common.hpp"
#include "tools.hpp"

#include <unistd.h>
#include <sys/wait.h>

extern PluginStartupInfo startup_info;

void errorMessage(char *msg)
{
	char *msgItems[]={"Error", "", "Ok"};
	msgItems[1] = msg;
	startup_info.Message(startup_info.ModuleNumber,
		FMSG_WARNING, NULL,
		msgItems,
		sizeof(msgItems)/sizeof(msgItems[0]), 1);
}

int messageBox(unsigned long flags, char **items, int noItems, int noButtons)
{
	return(startup_info.Message(startup_info.ModuleNumber,
		flags, NULL, items,
		noItems, noButtons));
}

char* point2name(char *path)
{
	char *p = path;
	while(*path)
	{
		if(*path == '\\' || *path == '/' || *path==':') p = path+1;
		path++;
	}
	return p;
}

WORD calculateCheckSum(BYTE* p)
{
	WORD sum = 0;
	for(int i = 0; i < sizeof(HoHdr)-2; ++i) sum += *p++;
	return 257*sum + 105;
}

bool isEnAlphaNum(BYTE ch)
{
	if(ch >= '0' && ch <= '9') return true;
	if(ch >= 'A' && ch <= 'Z') return true;
	if(ch >= 'a' && ch <= 'z') return true;
	return false;
}

bool isValidChar(BYTE ch)
{
	if(ch < 0x20 || ch > 0x7F) return false;
	BYTE invalidChar[] = {' ', '.', '\\', '/', ':', '?', '*', '<', '>', '|'};
	for(int i = 0; i < sizeof(invalidChar); i++)
		if(ch == invalidChar[i]) return false;
	return true;
}

void makeTrDosName(char* dest, const BYTE* hdr)
{
	*dest++ = '[';
	for(int i = 0; i < 8; ++i) *dest++ = hdr[i] ? hdr[i] : ' ';

	*dest++ = '.';

	BYTE ch1, ch2, ch3;
	ch1 = hdr[8];
	ch2 = hdr[9];
	ch3 = hdr[10];

	if(isEnAlphaNum(ch1) &&
		isEnAlphaNum(ch2) &&
		isEnAlphaNum(ch3))
	{
		*dest++ = ch1;
		*dest++ = ch2;
		*dest++ = ch3;
	}
	else
	{
		*dest++ = ch1 ? ch1 : ' ';
	}
	*dest++ = ']';
	*dest++ = 0;
}

void makePCName(char *from, char *to)
{
	int first = 0, last = 7;
	while(first <= last)
	{
		if(isValidChar(from[first])) break;
		++first;
	}
	while(last >= first)
	{
		if(isValidChar(from[last])) break;
		--last;
	}
	BYTE  noChars = 0;

	if(first > last)
	{
		memset(to, '_', 8);
		noChars = 8;
	}
	else
	{
		while(first <= last)
		{
			BYTE ch = from[first++];
			to[noChars++] = isValidChar(ch) ? ch : '_';
		}
	}

	// обрабатываем специальные имена устройств
	if(noChars == 3 || noChars == 4)
	{
		if(!memcmpi((const char *)to, "com", 3) ||
			!memcmpi((const char *)to, "lpt", 3) ||
			!memcmpi((const char *)to, "prn", 3) ||
			!memcmpi((const char *)to, "con", 3) ||
			!memcmpi((const char *)to, "aux", 3) ||
			!memcmpi((const char *)to, "nul", 3)) to[noChars++] = '_';
	}
        to[noChars] = 0;
}

int memcmpi(const char* p1, const char* p2, int maxlen)
{
	if(!maxlen) return(0);
	char ch1, ch2;

	while(maxlen--)
	{
		ch1 = *p1++;
		ch2 = *p2++;

		if (ch1 >= 'a' && ch1 <= 'z')
			ch1 -= 'a' - 'A';

		if (ch2 >= 'a' && ch2 <= 'z')
			ch2 -= 'a' - 'A';

		if(ch1 != ch2) break;
	}
	return (ch1 - ch2);
}

char* truncPathStr(char* newName, const char* name, int noChars)
{
	int size = strlen(name);
	if(size > noChars)
	{
		if(name[1] == ':' && name[2] == '/')
		{
			strncpy(newName, name, 4);
			strcat(newName, "...");
			strcat(newName, name+size-noChars+6);
		}
		else
		{
			strcpy(newName, "...");
			strcat(newName, name+size-noChars+3);
		}
	}
	else
		strcpy(newName, name);
	return newName;
}

FarDialogItem InitItem(int type, int x1, int y1, int x2, int y2, int focus, const void* param, DWORD flags, int def, const char* data)
{
	FarDialogItem item = {type, x1, y1, x2, y2, focus, (DWORD_PTR)param, flags, def};
        snprintf(item.Data, sizeof(item.Data), "%s", data ? data : "");

        return item;
}

