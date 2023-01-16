#ifndef xTRD_HPP
#define xTRD_HPP
#include "far2sdk/farplug-mb.h"
using namespace oldfar;
#include "types.hpp"

extern "C" {
  void   SetStartupInfo   (PluginStartupInfo *info);
  HANDLE OpenFilePlugin   (char *name, const unsigned char *data, int DataSize);
  HANDLE OpenPlugin       (int OpenFrom, int Item);
  void   ClosePlugin      (HANDLE hPlugin);
  void   GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *info);
  int    GetFindData      (HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pNoItems, int opMode);
  void   FreeFindData     (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems);
  void   ExitFAR();
  int    DeleteFiles      (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int opMode);
  int    PutFiles         (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int move, int opMode);
  int    GetFiles         (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int move, char *destPath, int opMode);
  int    SetDirectory     (HANDLE hPlugin, char* dirName, int opMode);
  int    MakeDirectory    (HANDLE hPlugin, char *name, int opMode);
  void   GetPluginInfo    (PluginInfo *info);
  int    Configure        (int itemNum);
  int    ProcessEvent     (HANDLE hPlugin, int event, void *param);
  int    ProcessKey       (HANDLE hPlugin, int key, unsigned int controlState);
  int    ProcessHostFile  (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int opMode);
}

#endif
