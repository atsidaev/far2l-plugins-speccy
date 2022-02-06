#include <windows.h>

#include "xTRD.hpp"
#include "manager.hpp"
#include "FmtReader.hpp"
#include "registry.hpp"
#include "detector.hpp"
#include "tools.hpp"
#include "types.hpp"
#include "lang.hpp"

#include <string>
#include "widestring.hpp"

PluginStartupInfo    startupInfo;
Options              op;

FmtReader* fmtReader;
Registry*  reg;
Detector*  detector;

void WINAPI _export SetStartupInfo(PluginStartupInfo *info)
{
  // получили свою копию PluginStartupInfo
  startupInfo = *info;

  reg                    = new Registry((char*)startupInfo.RootKey, L"/ZX/xTRD/");
  
  op.reread              = false;
  op.showExt             = reg->getNumber(HKEY_CURRENT_USER, L"ShowExt",            1);
  op.defaultPanelMode    = reg->getNumber(HKEY_CURRENT_USER, L"DefaultPanelMode",   5);
  op.defaultFormat       = reg->getNumber(HKEY_CURRENT_USER, L"DefaultFormat",      1);
  op.detectFormat        = reg->getNumber(HKEY_CURRENT_USER, L"DetectFormat",       1);
  op.autoMove            = reg->getNumber(HKEY_CURRENT_USER, L"AutoMove",           1);
  op.useDS               = reg->getNumber(HKEY_CURRENT_USER, L"UseDS",              1);

  char cmdLine1[300] = "c:\\dir1\\emul1.exe %P [%N]";
  char cmdLine2[300] = "c:\\dir2\\emul2.exe %d \"%f\"";
  char types1[100] = "Bb";
  char types2[100] = "*";
  reg->getString(HKEY_CURRENT_USER, L"CmdLine1", op.cmdLine1, cmdLine1, 300);
  reg->getString(HKEY_CURRENT_USER, L"CmdLine2", op.cmdLine2, cmdLine2, 300);
  reg->getString(HKEY_CURRENT_USER, L"Types1",   op.types1,   types1,   100);
  reg->getString(HKEY_CURRENT_USER, L"Types2",   op.types2,   types2,   100);
  op.fullScreen1 = reg->getNumber(HKEY_CURRENT_USER, L"FullScreen1", 0);
  op.fullScreen2 = reg->getNumber(HKEY_CURRENT_USER, L"FullScreen2", 0);

  char iniFilePath[300] = "";
  reg->getString(HKEY_CURRENT_USER, L"IniFilePath", op.iniFilePath, iniFilePath, 300);
  
  detector            = new Detector (startupInfo.ModuleName);
  fmtReader           = new FmtReader();
}

void WINAPI _export ExitFAR()
{
  delete reg;
  delete detector;
  delete fmtReader;
}

HANDLE WINAPI _export OpenFilePlugin(char *name, const unsigned char *data, int dataSize)
{
  if(name == NULL) return INVALID_HANDLE_VALUE;

  FmtPlugin* format = fmtReader->isImage(name, data, dataSize);
  if(format == NULL) return INVALID_HANDLE_VALUE;
    
  Manager *m = new Manager(name, format);
  return (HANDLE)m;
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom, int Item)
{
  if(OpenFrom != OPEN_COMMANDLINE) return INVALID_HANDLE_VALUE;
  char *fileName = (char*)Item;

  auto wideFileName = _W(fileName);

  HANDLE hostFile = CreateFile(wideFileName.c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
  
  BYTE data[16*sectorSize];
  if(GetFileSize(hostFile, NULL) < sizeof(data))
  {
    CloseHandle(hostFile);
    return INVALID_HANDLE_VALUE;
  }

  DWORD noBytesRead;
  ReadFile(hostFile, data, sizeof(data), &noBytesRead, NULL);
  CloseHandle(hostFile);
  
  FmtPlugin* format = fmtReader->isImage(fileName, data, sizeof(data));
  if(format == NULL) return INVALID_HANDLE_VALUE;
  
  Manager *m = new Manager(fileName, format);
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

  static char *cmdPrefix = "xTRD";
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
    reg->setNumber(HKEY_CURRENT_USER, L"DefaultPanelMode", panel.ViewMode);
  }

  return FALSE;
}

int WINAPI _export SetDirectory(HANDLE hPlugin, char* dirName, int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return m->setDirectory(dirName, opMode);
}

int WINAPI _export MakeDirectory(HANDLE hPlugin, char *name, int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return m->makeDirectory(name, opMode);
}

int WINAPI _export DeleteFiles (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int opMode)
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

int WINAPI _export PutFiles(HANDLE hPlugin,
                            PluginPanelItem *panelItem,
                            int noItems,
                            int move,
                            int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->putFiles(panelItem, noItems, move, opMode));
}

int WINAPI _export Configure(int itemNum)
{
  InitDialogItem items[] =
  {
    DI_DOUBLEBOX,3,1,60,23,0,0,0,0,(char*)MConfig,
    DI_CHECKBOX,5,2,0,0,TRUE,0,0,0,(char*)MDetectFormats,
    DI_CHECKBOX,5,3,0,0,0,0,0,0,(char*)MShowExt,
    DI_CHECKBOX,5,4,0,0,0,0,0,0,(char*)MAutoMove,
    DI_CHECKBOX,5,5,0,0,0,0,0,0,(char*)MUseDS,
    DI_TEXT,3,6,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_RADIOBUTTON,5,7,0,0,0,0,DIF_GROUP,0,(char*)MHoBeta,
    DI_RADIOBUTTON,5,8,0,0,0,0,0,0,(char*)MSCL,
    DI_RADIOBUTTON,5,9,0,0,0,0,0,0,(char*)MAutoSelect,
    DI_TEXT,3,10,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_TEXT,5,11,0,0,0,0,0,0,(char*)MEnterExec,
    DI_TEXT,5,12,0,0,0,0,0,0,(char*)MTypes,
    DI_EDIT,19,12,58,0,0,0,0,0,op.types1,
    DI_TEXT,5,13,0,0,0,0,0,0,(char*)MCmdLine,
    DI_EDIT,19,13,58,0,0,0,0,0,op.cmdLine1,
    DI_CHECKBOX,19,14,0,0,0,0,0,0,(char*)MFullScr,
    DI_TEXT,5,15,0,0,0,0,0,0,(char*)MCtrlEnterExec,
    DI_TEXT,5,16,0,0,0,0,0,0,(char*)MTypes,
    DI_EDIT,19,16,58,0,0,0,0,0,op.types2,
    DI_TEXT,5,17,0,0,0,0,0,0,(char*)MCmdLine,
    DI_EDIT,19,17,58,0,0,0,0,0,op.cmdLine2,
    DI_CHECKBOX,19,18,0,0,0,0,0,0,(char*)MFullScr,
    DI_TEXT,3,19,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_TEXT,5,20,0,0,0,0,0,0,(char*)MIniPath,
    DI_EDIT,25,20,58,0,0,0,0,0,op.iniFilePath,
    DI_TEXT,3,21,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,0,22,0,0,0,0,DIF_CENTERGROUP,1,(char*)MSave,
    DI_BUTTON,0,22,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
  };
  
  FarDialogItem dlgItems[sizeof(items)/sizeof(items[0])];
  initDialogItems(items, dlgItems, sizeof(items)/sizeof(items[0]));
  
  dlgItems[1].Selected = op.detectFormat;
  dlgItems[2].Selected = op.showExt;
  dlgItems[3].Selected = op.autoMove;
  dlgItems[4].Selected = op.useDS;

  dlgItems[15].Selected = op.fullScreen1;
  dlgItems[21].Selected = op.fullScreen2;

  int num = 5 + op.defaultFormat;
  if(num < 6 || num > 8) num = 8;
  dlgItems[num].Selected = TRUE;
  
  int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                   -1,-1,64,25,
                                   NULL,
                                   dlgItems,
                                   sizeof(dlgItems)/sizeof(dlgItems[0]));
  if(askCode != 26) return FALSE;
  
  op.detectFormat = dlgItems[1].Selected;
  op.showExt      = dlgItems[2].Selected;
  op.autoMove     = dlgItems[3].Selected;
  op.useDS        = dlgItems[4].Selected;
  
  op.defaultFormat = 3;
  if(dlgItems[6].Selected) op.defaultFormat = 1;
  if(dlgItems[7].Selected) op.defaultFormat = 2;

  strcpy(op.types1,   dlgItems[12].Data);
  strcpy(op.cmdLine1, dlgItems[14].Data);
  op.fullScreen1     = dlgItems[15].Selected;
  
  strcpy(op.types2,   dlgItems[18].Data);
  strcpy(op.cmdLine2, dlgItems[20].Data);
  op.fullScreen2     = dlgItems[21].Selected;

  strcpy(op.iniFilePath, dlgItems[24].Data);
  
  reg->setNumber(HKEY_CURRENT_USER, L"DetectFormat",  op.detectFormat);
  reg->setNumber(HKEY_CURRENT_USER, L"ShowExt",       op.showExt);
  reg->setNumber(HKEY_CURRENT_USER, L"DefaultFormat", op.defaultFormat);
  reg->setNumber(HKEY_CURRENT_USER, L"AutoMove",      op.autoMove);
  reg->setNumber(HKEY_CURRENT_USER, L"UseDS",         op.useDS);
  
  reg->setNumber(HKEY_CURRENT_USER, L"FullScreen1",   op.fullScreen1);
  reg->setNumber(HKEY_CURRENT_USER, L"FullScreen2",   op.fullScreen2);
  reg->setString(HKEY_CURRENT_USER, L"CmdLine1",      op.cmdLine1);
  reg->setString(HKEY_CURRENT_USER, L"CmdLine2",      op.cmdLine2);
  reg->setString(HKEY_CURRENT_USER, L"Types1",        op.types1);
  reg->setString(HKEY_CURRENT_USER, L"Types2",        op.types2);

  reg->setString(HKEY_CURRENT_USER, L"IniFilePath",   op.iniFilePath);
  
  return TRUE;
}

int WINAPI _export ProcessKey(HANDLE hPlugin, int key, unsigned int controlState)
{
  Manager* m = (Manager*)hPlugin;
  return (m->processKey(key, controlState));
}

int WINAPI _export ProcessHostFile(HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->processHostFile(panelItem, noItems, opMode));
}
