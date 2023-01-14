#include "XiSD.hpp"
#include "memory.hpp"
#include "manager.hpp"
#include "FmtReader.hpp"
#include "tools.hpp"
#include "registry.hpp"
#include "lang.hpp"

void* operator new   (size_t size) { return malloc(size); }
void  operator delete(void *block) { free(block); }

PluginStartupInfo startupInfo;
Registry*         reg;
FmtReader*        fmtReader;
Options           op;

void WINAPI _export SetStartupInfo(PluginStartupInfo *info)
{
  // получили свою копию PluginStartupInfo
  startupInfo = *info;
  fmtReader = new FmtReader(startupInfo.ModuleName);

  reg = new Registry(startupInfo.RootKey);
  op.defaultPanelMode = reg->getNumber(HKEY_CURRENT_USER, "DefaultPanelMode", 4);
  op.reread = false;

  char columnTypes [] = "N,S,C0,C1,C2";
  char columnWidths[] = "0,8,4,5,5";
  char *columnTitles = getMsg(MTitle);
  reg->getString(HKEY_CURRENT_USER, "ColumnTypes",  op.columnTypes,  columnTypes,  100);
  reg->getString(HKEY_CURRENT_USER, "ColumnWidths", op.columnWidths, columnWidths, 100);
  reg->getString(HKEY_CURRENT_USER, "ColumnTitles", op.columnTitles, columnTitles, 100);
}

void WINAPI _export ExitFAR()
{
  delete reg;
  delete fmtReader;
}

HANDLE WINAPI _export OpenFilePlugin(char* name, const u8* data, u16 dataSize)
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
  
  HANDLE hostFile = CreateFile(fileName,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(hostFile == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
  
  BYTE data[blockSize];
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
  Manager *m = (Manager*)hPlugin;
  delete m;
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

  static char *cmdPrefix = "XiSD";
  info->CommandPrefix = cmdPrefix;
  static char *pluginCfgStrings[1];
  pluginCfgStrings[0] = getMsg(MConfig);
  info->PluginConfigStrings = pluginCfgStrings;
  info->PluginConfigStringsNumber = 1;
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

int WINAPI _export SetDirectory(HANDLE hPlugin, char* dirName, int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return m->setDirectory(dirName, opMode);
}

int WINAPI _export GetFiles(HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int move, char *destPath, int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->getFiles(panelItem, noItems, move, destPath, opMode));
}

int WINAPI _export DeleteFiles (HANDLE hPlugin, PluginPanelItem *panelItem, int noItems, int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return (m->deleteFiles(panelItem, noItems, opMode));
}

int WINAPI _export Configure(int itemNum)
{
  char defaultPanelModeString[2] = " ";
  defaultPanelModeString[0] = '0' + op.defaultPanelMode;
  InitDialogItem items[] =
  {
    DI_DOUBLEBOX,3,1,50,10,0,0,0,0,(char*)MConfig,
    DI_TEXT,5,2,0,0,0,0,0,0,(char *)MDefPanelMode,
    DI_EDIT,47,2,48,2,TRUE,0,0,0,defaultPanelModeString,
    DI_TEXT,3,3,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_TEXT,5,4,0,0,0,0,0,0,(char *)MMode4Settings,
    DI_TEXT,5,5,0,0,0,0,0,0,(char *)MColumnTypes,
    DI_EDIT,23,5,48,5,0,0,0,0,op.columnTypes,
    DI_TEXT,5,6,0,0,0,0,0,0,(char *)MColumnWidths,
    DI_EDIT,23,6,48,6,0,0,0,0,op.columnWidths,
    DI_TEXT,5,7,0,0,0,0,0,0,(char *)MColumnTitles,
    DI_EDIT,23,7,48,7,0,0,0,0,op.columnTitles,
    DI_TEXT,3,8,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    DI_BUTTON,0,9,0,0,0,0,DIF_CENTERGROUP,1,(char *)MSave,
    DI_BUTTON,0,9,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
  };
  
  FarDialogItem dlgItems[sizeof(items)/sizeof(items[0])];
  initDialogItems(items, dlgItems, sizeof(items)/sizeof(items[0]));
  
  int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                   -1,-1,54,12,
                                   NULL,
                                   dlgItems,
                                   sizeof(dlgItems)/sizeof(dlgItems[0]));
  if(askCode != 12) return FALSE;

  int mode = dlgItems[2].Data[0] - '0';
  if(mode < 0 || mode > 9) mode = 4;
  op.defaultPanelMode = mode;

  lstrcpy(op.columnTypes,  dlgItems[6].Data);
  lstrcpy(op.columnWidths, dlgItems[8].Data);
  lstrcpy(op.columnTitles, dlgItems[10].Data);
  
  reg->setNumber(HKEY_CURRENT_USER, "DefaultPanelMode", op.defaultPanelMode);
  reg->setString(HKEY_CURRENT_USER, "ColumnTypes",      op.columnTypes);
  reg->setString(HKEY_CURRENT_USER, "ColumnWidths",     op.columnWidths);
  reg->setString(HKEY_CURRENT_USER, "ColumnTitles",     op.columnTitles);

  return TRUE;
}

int WINAPI _export MakeDirectory(HANDLE hPlugin, char *name, int opMode)
{
  Manager* m = (Manager*)hPlugin;
  return m->makeDirectory(name, opMode);
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

int WINAPI _export ProcessKey(HANDLE hPlugin, int key, unsigned int controlState)
{
  Manager* m = (Manager*)hPlugin;
  return (m->processKey(key, controlState));
}
