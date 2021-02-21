#ifndef xCREATE_HPP
#define xCREATE_HPP
#include <windows.h>

extern "C"
{
  void   WINAPI _export SetStartupInfo(PluginStartupInfo* info_);
  HANDLE WINAPI _export OpenPlugin    (int openFrom, int item);
  void   WINAPI _export GetPluginInfo (PluginInfo* info);
};

#endif