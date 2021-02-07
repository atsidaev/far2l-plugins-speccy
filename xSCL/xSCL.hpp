#ifndef xSCL_HPP
#define xSCL_HPP
#include "plugin.hpp"

extern "C" {
  void   WINAPI _export SetStartupInfo   (PluginStartupInfo *info);
  HANDLE WINAPI _export OpenFilePlugin   (char *name, const unsigned char *data, int DataSize);
  HANDLE WINAPI _export OpenPlugin       (int OpenFrom, int Item);
  void   WINAPI _export ClosePlugin      (HANDLE hPlugin);
  void   WINAPI _export GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *info);
  int    WINAPI _export GetFindData      (HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pNoItems, int opMode);
  void   WINAPI _export FreeFindData     (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems);
  void   WINAPI _export ExitFAR();
  int    WINAPI _export DeleteFiles      (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int opMode);
  int    WINAPI _export PutFiles         (HANDLE hPlugin, PluginPanelItem *panelItem,int noItems,int move,int opMode);
  int    WINAPI _export GetFiles         (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int move, char *destPath, int opMode);
  void   WINAPI _export GetPluginInfo    (PluginInfo *info);
  int    WINAPI _export Configure        (int itemNum);
  int    WINAPI _export ProcessEvent     (HANDLE hPlugin, int event, void *param);
  //  int    WINAPI _export ProcessHostFile(HANDLE hPlugin,struct PluginPanelItem *panelItem,int noItems,int opMode);
  int    WINAPI _export ProcessKey       (HANDLE hPlugin, int key, unsigned int controlState);
}

#endif
