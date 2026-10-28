#include <windows.h>
#include <memory>

#include "xHRiP.hpp"
#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "registry.hpp"
#include "detector.hpp"
#include "lang.hpp"
#include "widestring.hpp"

PluginStartupInfo    startupInfo;
Options              op;
Registry*            reg;
Detector*            detector;

void WINAPI _export SetStartupInfo(PluginStartupInfo *info)
{
  // получили свою копию PluginStartupInfo
  startupInfo         = *info;

  reg                 = new Registry((char*)startupInfo.RootKey, L"/ZX/xHRiP/");
  op.showExt          = reg->getNumber(HKEY_CURRENT_USER, L"ShowExt", 1);
  op.defaultPanelMode = reg->getNumber(HKEY_CURRENT_USER, L"DefaultPanelMode", 5);
  op.defaultFormat    = reg->getNumber(HKEY_CURRENT_USER, L"DefaultFormat", 1);
  op.detectFormat     = reg->getNumber(HKEY_CURRENT_USER, L"DetectFormat", 1);

  char iniFilePath[sizeof(op.iniFilePath)] = "";
  reg->getString(HKEY_CURRENT_USER, L"IniFilePath", op.iniFilePath, iniFilePath,
                 sizeof(op.iniFilePath));

  detector            = new Detector ((char*)startupInfo.ModuleName);
}

void WINAPI _export ExitFAR()
{
  delete detector;
  delete reg;
}

HANDLE WINAPI _export OpenFilePlugin(char * name, const unsigned char * data, int size) {
  if(!name || !data || size < 17 || speccy::le16(data + 15) != speccy::hobetaChecksum(data))
    return INVALID_HANDLE_VALUE;
  try {
    for(int offset = 17; offset <= size - 13; ++offset) {
      if(memcmp(data + offset, "HRi", 3) || memcmp(data + offset + 8, "Hrst2", 5))
        continue;
      std::unique_ptr<Manager> manager(new Manager(name, offset));
      if(manager->isValid())
        return manager.release();
    }
  } catch(const std::exception &) {
  }
  return INVALID_HANDLE_VALUE;
}

HANDLE WINAPI _export OpenPlugin(int from, INT_PTR item) {
  if(from != OPEN_COMMANDLINE || !item)
    return INVALID_HANDLE_VALUE;
  std::string name = reinterpret_cast<const char *>(item);
  auto begin = name.find_first_not_of(" \t"), end = name.find_last_not_of(" \t");
  if(begin == std::string::npos)
    return INVALID_HANDLE_VALUE;
  name = name.substr(begin, end - begin + 1);
  if(name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  speccy::Bytes bytes;
  if(!speccy::readFile(name, bytes))
    return INVALID_HANDLE_VALUE;
  return OpenFilePlugin(&name[0], bytes.data(), bytes.size());
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

  static char *cmdPrefix = "xHRiP";
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
                                   "Config",
                                   dlgItems,
                                   sizeof(dlgItems)/sizeof(dlgItems[0]));
  if(askCode != 11) return FALSE;
  
  op.detectFormat = dlgItems[1].Selected;
  op.showExt      = dlgItems[2].Selected;

  op.defaultFormat = 3;
  if(dlgItems[4].Selected) op.defaultFormat = 1;
  if(dlgItems[5].Selected) op.defaultFormat = 2;

  snprintf(op.iniFilePath, sizeof(op.iniFilePath), "%s", dlgItems[9].Data);

  reg->setNumber(HKEY_CURRENT_USER, L"DetectFormat",  op.detectFormat);
  reg->setNumber(HKEY_CURRENT_USER, L"ShowExt",       op.showExt);
  reg->setNumber(HKEY_CURRENT_USER, L"DefaultFormat", op.defaultFormat);
  reg->setString(HKEY_CURRENT_USER, L"IniFilePath",   op.iniFilePath);
  delete detector;
  detector = new Detector(startupInfo.ModuleName);

  return TRUE;
}
