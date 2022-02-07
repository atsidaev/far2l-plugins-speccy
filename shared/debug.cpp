#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

extern PluginStartupInfo startupInfo;

void errorMsg(char *msg)
{
  char *msgItems[]={"Error", "", "Ok"};
  msgItems[1] = msg;
  startupInfo.Message(startupInfo.ModuleNumber,
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
  startupInfo.Message(startupInfo.ModuleNumber,
                      FMSG_WARNING,
                      NULL,
                      msgItems,
                      sizeof(msgItems)/sizeof(msgItems[0]), 1);
}
