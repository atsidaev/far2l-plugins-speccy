#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "debug.hpp"

extern PluginStartupInfo startup_info;

void errorMsg(char *msg)
{
	char *msgItems[]={"Error", "", "Ok"};
	msgItems[1] = msg;
	startup_info.Message(startup_info.ModuleNumber,
		FMSG_WARNING, NULL,
		msgItems,
		sizeof(msgItems)/sizeof(msgItems[0]), 1);
}

void debugMsg(char *msg, int arg)
{
	char *msgItems[]={"Debug", "", "", "Ok"};
	char line2[40];
	sprintf(line2, "%d   0x%x", arg, arg);
	msgItems[1] = msg;
	msgItems[2] = line2;
	startup_info.Message(startup_info.ModuleNumber,
		FMSG_WARNING,
		NULL,
		msgItems,
		sizeof(msgItems)/sizeof(msgItems[0]), 1);
}
