#include <windows.h>
#include <far2sdk/farplug-mb.h>
using namespace oldfar;
#include "xCreate.hpp"
#include "registry.hpp"
#include "tools.hpp"
#include "creator.hpp"
#include "lang.hpp"

PluginStartupInfo info;

Registry*  reg; 
char   BOOTFile[256];
char   comment[256];
int    isDS;
int    imageType;
int    isWriteBOOT;
int    format;
int    interleave;
int    writeProtect;
int    isComment;

char   title[12];
char   fileName[256];

void WINAPI _export SetStartupInfo(PluginStartupInfo *info_)
{
  info = *info_;
  reg = new Registry((char*)info.RootKey, L"/ZX/xCreate"); 

  isDS         = reg->getNumber(HKEY_CURRENT_USER, L"IsDS", 1);
  isWriteBOOT  = reg->getNumber(HKEY_CURRENT_USER, L"IsWriteBOOT", 0);
  format       = reg->getNumber(HKEY_CURRENT_USER, L"DefaultFormat", FMT_TRD);
  interleave   = reg->getNumber(HKEY_CURRENT_USER, L"Interleave", 2);
  writeProtect = reg->getNumber(HKEY_CURRENT_USER, L"WriteProtect", 0);
  isComment    = reg->getNumber(HKEY_CURRENT_USER, L"IsComment", 1);
  reg->getString(HKEY_CURRENT_USER, L"DefaultBOOT", BOOTFile, "", sizeof(BOOTFile));
  reg->getString(HKEY_CURRENT_USER, L"DefaultComment", comment, getMsg(MDefComment), sizeof(BOOTFile));
}

HANDLE WINAPI _export OpenPlugin(int openFrom, int item)
{
  InitDialogItem initItems[] =
  {
    /*0*/DI_DOUBLEBOX,3,1,63,23,0,0,0,0,(char *)MCreate,
    /*1*/DI_TEXT,5,2,0,0,0,0,0,0,(char *)MFileName,
    /*2*/DI_EDIT,5,3,61,0,TRUE,(DWORD_PTR)"xCreateFileName",DIF_HISTORY,0,"",

    /*3*/DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    /*4*/DI_TEXT,5,5,0,0,0,0,0,0,(char *)MFileFormat,
    /*5*/DI_RADIOBUTTON,5,6,0,0,0,0,DIF_GROUP,0,(char *)MTRD,
    /*6*/DI_RADIOBUTTON,14,6,0,0,0,0,0,0,(char *)MFDI,
    /*7*/DI_RADIOBUTTON,23,6,0,0,0,0,0,0,(char *)MSCL,
    /*8*/DI_RADIOBUTTON,32,6,0,0,0,0,0,0,(char *)MFDD,
    /*9*/DI_RADIOBUTTON,41,6,0,0,0,0,0,0,(char *)MUDI,
    /*10*/DI_RADIOBUTTON,50,6,0,0,0,0,0,0,(char *)MTeleDisk,

    /*11*/DI_TEXT,3,7,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    /*12*/DI_TEXT,5,8,0,0,0,0,0,0,(char *)MTitle,
    /*13*/DI_EDIT,5,9,16,0,0,(DWORD_PTR)"xCreateTitle",DIF_HISTORY,0,"",

    /*14*/DI_TEXT,3,10,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    /*15*/DI_TEXT,5,11,0,0,0,0,0,0,(char *)MInterleave,
	/*16*/DI_RADIOBUTTON,5,12,0,0,0,0,DIF_GROUP,0,(char *)MInterleave1,
	/*17*/DI_RADIOBUTTON,5,13,0,0,0,0,0,0,(char *)MInterleave2,

    /*18*/DI_TEXT,3,14,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    /*19*/DI_CHECKBOX,5,15,0,0,0,0,0,0,(char *)MWriteProtect,
    /*20*/DI_CHECKBOX,5,16,0,0,0,0,0,0,(char *)MInstallDS,
    /*21*/DI_CHECKBOX,5,17,0,0,0,0,0,0,(char *)MWriteBOOT,
    /*22*/DI_EDIT,5,18,61,0,0,(DWORD_PTR)"xCreateBOOTName",DIF_HISTORY,0,BOOTFile,
    /*23*/DI_CHECKBOX,5,19,0,0,0,0,0,0,(char *)MComment,
    /*24*/DI_EDIT,5,20,61,0,0,(DWORD_PTR)"xCreateComment",DIF_HISTORY,0,comment,

    /*25*/DI_TEXT,3,21,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
    /*26*/DI_BUTTON,0,22,0,0,0,0,DIF_CENTERGROUP,1,(char *)MOk,
    /*27*/DI_BUTTON,0,22,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
  };
  
  FarDialogItem dialogItems[sizeof(initItems)/sizeof(initItems[0])];

  initDialogItems(initItems, dialogItems, sizeof(initItems)/sizeof(initItems[0]));
  dialogItems[19].Selected = writeProtect;
  dialogItems[20].Selected = isDS;
  dialogItems[21].Selected = isWriteBOOT;
  dialogItems[23].Selected = isComment;
  if(interleave == 1)
    dialogItems[16].Selected = TRUE;
  else
    dialogItems[17].Selected = TRUE;
  switch(format)
  {
    case FMT_TRD:
              dialogItems[5].Selected = TRUE;
              break;
    case FMT_FDI:
              dialogItems[6].Selected = TRUE;
              break;
    case FMT_SCL:
              dialogItems[7].Selected = TRUE;
              break;
    case FMT_FDD:
              dialogItems[8].Selected = TRUE;
              break;
    case FMT_UDI:
              dialogItems[9].Selected = TRUE;
              break;
    case FMT_TD:
              dialogItems[10].Selected = TRUE;
              break;
    default:
              dialogItems[5].Selected = TRUE;
  }
  int exitCode = info.Dialog(info.ModuleNumber,
                             -1,-1,67,25,
                             NULL,
                             dialogItems,
                             sizeof(dialogItems)/sizeof(dialogItems[0]));
  
  if(exitCode != 26) return (INVALID_HANDLE_VALUE);

  writeProtect = dialogItems[19].Selected ? 1 : 0;
  isDS         = dialogItems[20].Selected ? 1 : 0;
  isWriteBOOT  = dialogItems[21].Selected ? 1 : 0;
  isComment    = dialogItems[23].Selected ? 1 : 0;
  if(dialogItems[16].Selected) interleave = 1;
  if(dialogItems[17].Selected) interleave = 2;
  if(dialogItems[5].Selected) format = FMT_TRD;
  if(dialogItems[6].Selected) format = FMT_FDI;
  if(dialogItems[7].Selected) format = FMT_SCL;
  if(dialogItems[8].Selected) format = FMT_FDD;
  if(dialogItems[9].Selected) format = FMT_UDI;
  if(dialogItems[10].Selected) format = FMT_TD;
  strcpy(comment,  dialogItems[24].Data);
  strcpy(BOOTFile, dialogItems[22].Data);
  strcpy(title,    dialogItems[13].Data);
  strcpy(fileName, dialogItems[2].Data);
  
  reg->setString(HKEY_CURRENT_USER, L"DefaultComment", comment);
  reg->setString(HKEY_CURRENT_USER, L"DefaultBOOT",    BOOTFile);
  reg->setNumber(HKEY_CURRENT_USER, L"IsDS",           isDS);
  reg->setNumber(HKEY_CURRENT_USER, L"IsWriteBOOT",    isWriteBOOT);
  reg->setNumber(HKEY_CURRENT_USER, L"DefaultFormat",  format);
  reg->setNumber(HKEY_CURRENT_USER, L"Interleave",     interleave);
  reg->setNumber(HKEY_CURRENT_USER, L"WriteProtect",   writeProtect);
  reg->setNumber(HKEY_CURRENT_USER, L"IsComment",      isComment);

  create(fileName, title, writeProtect, isDS, format, interleave, isWriteBOOT?BOOTFile:0, isComment&&comment[0]?comment:0);

  info.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEPANEL, NULL);
  info.Control(INVALID_HANDLE_VALUE, FCTL_REDRAWPANEL, NULL);
  info.Control(INVALID_HANDLE_VALUE, FCTL_UPDATEANOTHERPANEL, NULL);
  info.Control(INVALID_HANDLE_VALUE, FCTL_REDRAWANOTHERPANEL, NULL);

  return INVALID_HANDLE_VALUE;
}

void WINAPI _export GetPluginInfo(PluginInfo *info)
{
  info->StructSize = sizeof(*info);
  info->Flags = 0;
  info->DiskMenuStringsNumber = 0;
  static char *menuStrings[1];
  menuStrings[0] = getMsg(MCreate);
  
  info->PluginMenuStrings         = menuStrings;
  info->PluginMenuStringsNumber   = sizeof(menuStrings)/sizeof(menuStrings[0]);
  info->PluginConfigStringsNumber = 0;
}
