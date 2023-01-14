#ifndef XiSD_hpp
#define XiSD_hpp
#include "plugin.hpp"
#include "types.hpp"

extern "C" {
  void   WINAPI _export SetStartupInfo   (PluginStartupInfo *info);
  HANDLE WINAPI _export OpenFilePlugin   (char *name, const u8* data, u16 DataSize);
  HANDLE WINAPI _export OpenPlugin       (int OpenFrom, int Item);
  void   WINAPI _export ClosePlugin      (HANDLE hPlugin);
  void   WINAPI _export GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *info);
  void   WINAPI _export GetPluginInfo    (PluginInfo *info);
  int    WINAPI _export GetFindData      (HANDLE hPlugin, PluginPanelItem **pPanelItem, int *pNoItems, int opMode);
  void   WINAPI _export FreeFindData     (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems);
  void   WINAPI _export ExitFAR          ();

  int    WINAPI _export SetDirectory     (HANDLE hPlugin, char* dirName, int opMode);
  int    WINAPI _export GetFiles         (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int move, char *destPath, int opMode);
  int    WINAPI _export DeleteFiles      (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int opMode);
  int    WINAPI _export Configure        (int itemNum);
  int    WINAPI _export MakeDirectory    (HANDLE hPlugin, char *name, int opMode);
  int    WINAPI _export PutFiles         (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int move, int opMode);
  int    WINAPI _export ProcessKey       (HANDLE hPlugin, int key, unsigned int controlState);  
}

#endif
