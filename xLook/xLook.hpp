#ifndef XLOOK_HPP
#define XLOOK_HPP
#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

extern "C" {
  void   WINAPI _export SetStartupInfo   (const struct PluginStartupInfo *info);
  void   WINAPI _export ExitFAR          ();
  void   WINAPI _export GetPluginInfo    (struct PluginInfo *info);
  void   WINAPI _export GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *Info);
  int    WINAPI _export GetFindData      (HANDLE hPlugin, struct PluginPanelItem **pPanelItem, int *pItemsNumber, int OpMode);
  void   WINAPI _export FreeFindData     (HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber);
  int    WINAPI _export GetFiles         (HANDLE hPlugin, struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, char *DestPath, int OpMode);
  HANDLE WINAPI _export OpenFilePlugin   (char *Name, const unsigned char *Data, int DataSize);
  void   WINAPI _export ClosePlugin      (HANDLE hPlugin);
  HANDLE WINAPI _export OpenPlugin       (int open_from, INT_PTR item);
}

#endif
