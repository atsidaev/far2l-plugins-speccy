#include <windows.h>

#include "xSCL.hpp"
#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "registry.hpp"
#include "detector.hpp"
#include "lang.hpp"

PluginStartupInfo startupInfo;
Options           op;
Registry*         reg;
Detector*         detector;

void WINAPI _export SetStartupInfo(PluginStartupInfo *info)
{
  // получили свою копию PluginStartupInfo
  startupInfo = *info;

  reg =  new Registry(startupInfo.RootKey);
  op.showExt          = reg->getNumber(HKEY_CURRENT_USER, "ShowExt", 1);
  op.defaultPanelMode = reg->getNumber(HKEY_CURRENT_USER, "DefaultPanelMode", 5);
  op.defaultFormat    = reg->getNumber(HKEY_CURRENT_USER, "DefaultFormat", 1);
  op.detectFormat     = reg->getNumber(HKEY_CURRENT_USER, "DetectFormat", 1);
  op.reread           = false;

  char iniFilePath[300] = "";
  reg->getString(HKEY_CURRENT_USER, "IniFilePath", op.iniFilePath, iniFilePath, 300);

  detector = new Detector(startupInfo.ModuleName);
}

void WINAPI _export ExitFAR()
{
  delete reg;
  delete detector;
}

BYTE signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R' };

HANDLE WINAPI _export OpenFilePlugin(char *name, const unsigned char *data, int dataSize)
{
  if(name == NULL) return INVALID_HANDLE_VALUE;
  if(dataSize <= 9) return INVALID_HANDLE_VALUE;

  if(dataSize < 9+data[8]*sizeof(FileHdr) ||
     !compareMemory((unsigned char*)data, signature, sizeof(signature))) return INVALID_HANDLE_VALUE;

  HANDLE hostFile = CreateFile(name,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

  DWORD fileSize = GetFileSize(hostFile, NULL);
  DWORD noBytesRead;
  DWORD totalSecs = 0;

  SetFilePointer(hostFile, 9, NULL, FILE_BEGIN);
  for(int i = 0; i < data[8]; ++i)
  {
    FileHdr fHdr;
    ReadFile(hostFile, &fHdr, sizeof(fHdr), &noBytesRead, 0);
    totalSecs += fHdr.noSecs;
  }
  CloseHandle(hostFile);

  if(fileSize < totalSecs*sectorSize+data[8]*sizeof(FileHdr)+8+1+4)
    return INVALID_HANDLE_VALUE;

  Manager *m = new Manager(name);
  return (HANDLE)m;
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom, int Item)
{
  if(OpenFrom != OPEN_COMMANDLINE) return INVALID_HANDLE_VALUE;
  char *name = (char*)Item;

  HANDLE hostFile = CreateFile(name,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
  
  DWORD fileSize = GetFileSize(hostFile, NULL);
  
  DWORD noBytesRead;
  
  BYTE buf[sizeof(signature)];
  ReadFile(hostFile, buf, sizeof(buf), &noBytesRead, 0);
  BYTE no_files;
  ReadFile(hostFile, &no_files, 1, &noBytesRead, 0);
  
  if(fileSize < no_files*(sectorSize+sizeof(FileHdr))+8+1+4 ||
     !compareMemory(buf, signature, sizeof(signature)))
  {
    CloseHandle(hostFile);
    return INVALID_HANDLE_VALUE;
  }
  
  DWORD totalSecs = 0;
  for(int i = 0; i < no_files; ++i)
  {
    FileHdr fHdr;
    ReadFile(hostFile, &fHdr, sizeof(fHdr), &noBytesRead, 0);
    totalSecs += fHdr.noSecs;
  }
  
  if(fileSize < totalSecs*sectorSize+no_files*sizeof(FileHdr)+8+1+4)
  {
    CloseHandle(hostFile);
    return INVALID_HANDLE_VALUE;
  }
  CloseHandle(hostFile);
  Manager *m = new Manager(name);
  return (HANDLE)m;
}

void WINAPI _export ClosePlugin(HANDLE hPlugin)
{
  delete (Manager*)hPlugin;
}

int WINAPI _export GetFindData(HANDLE hPlugin,
                               PluginPanelItem **pPanelItem,
                               int *pNoItems,
                               int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->getFindData(pPanelItem, pNoItems, opMode));
}

void WINAPI _export FreeFindData(HANDLE hPlugin,
                                 PluginPanelItem *panelItem,
                                 int noItems)
{
  Manager* m = (Manager*)hPlugin;
  m->freeFindData(panelItem, noItems);
}

void WINAPI _export GetOpenPluginInfo(HANDLE hPlugin, OpenPluginInfo *info)
{
  Manager* m = (Manager*)hPlugin;
  m->getOpenPluginInfo(info);
}

void WINAPI _export GetPluginInfo(PluginInfo *info)
{
  info->StructSize = sizeof(*info);
  info->Flags      = PF_DISABLEPANELS;

  static char *cmdPrefix = "xSCL";
  info->CommandPrefix = cmdPrefix;
  static char *pluginCfgStrings[1];
  pluginCfgStrings[0] = getMsg(MConfig);
  info->PluginConfigStrings = pluginCfgStrings;
  info->PluginConfigStringsNumber = 1;
}

int WINAPI _export ProcessEvent(HANDLE hPlugin, int event, void *param)
{
  if(event == FE_CHANGEVIEWMODE)
  {
    startupInfo.Control(hPlugin, FCTL_UPDATEPANEL, (void*)1);
  }
  if(event == FE_CLOSE)
  {
    PanelInfo panel;
    startupInfo.Control(hPlugin, FCTL_GETPANELINFO, &panel);

    op.defaultPanelMode = panel.ViewMode;
    reg->setNumber(HKEY_CURRENT_USER, "DefaultPanelMode", panel.ViewMode);
  }
  return FALSE;
}

int WINAPI _export DeleteFiles(HANDLE hPlugin,
                               PluginPanelItem *panelItem,
                               int noItems,
                               int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->deleteFiles(panelItem, noItems, opMode));
}

int WINAPI _export GetFiles(HANDLE hPlugin,
                            PluginPanelItem *panelItem,
                            int noItems,
                            int move,
                            char *destPath,
                            int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->getFiles(panelItem, noItems, move, destPath, opMode));
}

int WINAPI _export PutFiles(HANDLE hPlugin,struct PluginPanelItem *panelItem,int noItems,int move,int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->putFiles(panelItem, noItems, move, opMode));
}

int WINAPI _export Configure(int itemNum)
{
  InitDialogItem items[] =
  {
    DI_DOUBLEBOX,3,1,40,13,0,0,0,0,(char*)MConfig,
    DI_CHECKBOX,5,2,0,0,TRUE,0,0,0,(char*)MDetectFormats,
    DI_CHECKBOX,5,3,0,0,0,0,0,0,(char*)MShowExt,
    DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_RADIOBUTTON,5,5,0,0,0,0,DIF_GROUP,0,(char*)MHoBeta,
    DI_RADIOBUTTON,5,6,0,0,0,0,0,0,(char*)MSCL,
    DI_RADIOBUTTON,5,7,0,0,0,0,0,0,(char*)MAutoSelect,
    DI_TEXT,3,8,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_TEXT,5,9,0,0,0,0,0,0,(char*)MIniPath,
    DI_EDIT,5,10,38,0,0,0,0,0,op.iniFilePath,
    DI_TEXT,3,11,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,0,12,0,0,0,0,DIF_CENTERGROUP,1,(char*)MSave,
    DI_BUTTON,0,12,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
  };
  
  FarDialogItem dlgItems[sizeof(items)/sizeof(items[0])];
  initDialogItems(items, dlgItems, sizeof(items)/sizeof(items[0]));
  
  dlgItems[1].Selected = op.detectFormat;
  dlgItems[2].Selected = op.showExt;
  int num = 3 + op.defaultFormat;
  if(num < 4 || num > 6) num = 6;
  dlgItems[num].Selected = TRUE;
  
  int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                   -1,-1,44,15,
                                   NULL,
                                   dlgItems,
                                   sizeof(dlgItems)/sizeof(dlgItems[0]));
  if(askCode != 11) return FALSE;
  
  op.detectFormat = dlgItems[1].Selected;
  op.showExt      = dlgItems[2].Selected;

  op.defaultFormat = 3;
  if(dlgItems[4].Selected) op.defaultFormat = 1;
  if(dlgItems[5].Selected) op.defaultFormat = 2;

  lstrcpy(op.iniFilePath, dlgItems[9].Data);

  reg->setNumber(HKEY_CURRENT_USER, "DetectFormat",  op.detectFormat);
  reg->setNumber(HKEY_CURRENT_USER, "ShowExt",       op.showExt);
  reg->setNumber(HKEY_CURRENT_USER, "DefaultFormat", op.defaultFormat);
  reg->setString(HKEY_CURRENT_USER, "IniFilePath",   op.iniFilePath);

  return TRUE;
}

int WINAPI _export ProcessKey(HANDLE hPlugin, int key, unsigned int controlState)
{
  Manager* m = (Manager*)hPlugin;
  return (m->processKey(key, controlState));
}
