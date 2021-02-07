#include <windows.h>
#include "plugin.hpp"

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;

const DWORD PKF_ANY = PKF_CONTROL | PKF_SHIFT | PKF_ALT;

int str2int(char* p, int len)
{
  int result = -1;
  while(*p == ' ') ++p;
  
  for(int i = 0; i < len; ++i)
  {
    BYTE digit = 0xFF;
    if(*p >= '0' && *p <= '9') digit = *p - '0';

    if(digit == 0xFF) break;
    if(result == -1) result = 0;
    
    result = 10*result + digit;
    p++;
  }
  return result;
}

int Manager::processKey(int key, unsigned int controlState)
{
  if(((controlState & PKF_SHIFT) && key == VK_F6) ||
     (!(controlState & PKF_ANY) && key == VK_F4) ||
     ((controlState & PKF_CONTROL) && key == 0x41))
  {
    if(isReadOnly)
    {
      char *msgItems[4];
      msgItems [0] = getMsg(MWarning);
      msgItems [1] = getMsg(MCanNotEdit);
      msgItems [2] = getMsg(MWriteProtected);
      msgItems [3] = getMsg(MOk);
      messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
      return TRUE;
    }
    HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
    PanelInfo info;
    startupInfo.Control(this,FCTL_GETPANELINFO,&info);
    startupInfo.RestoreScreen(screen);
    
    if(info.ItemsNumber > 0 && info.CurrentItem > 0)
    {
      char *fName = info.PanelItems[info.CurrentItem].FindData.cFileName;
      int fNum = 0;
      for(; fNum < noFiles; ++fNum)
        if(!lstrcmp(fName, pcFiles[fNum].name)) break;

      DWORD oldCheckSum = calculateCheckSum((BYTE*)&files[fNum], sizeof(FileHdr));
      
      char name[9] = "        ";
      for(int i = 0; i < 8; i++)
        name[i] = files[fNum].name[i] ? files[fNum].name[i] : ' ';
      char type[2] = " ";
      type[0] = files[fNum].type ? files[fNum].type : ' ';
      char start[6];
      wsprintf(start, "%d", files[fNum].start);
      
      InitDialogItem items[]=
      {
        DI_DOUBLEBOX,3,1,34,7,0,0,0,0,(char*)MEditFileInfo,
        DI_TEXT,5,2,0,0,0,0,0,0,(char*)MEditName,
        DI_EDIT,24,2,32,2,1,0,0,0,name,
        DI_TEXT,5,3,0,0,0,0,0,0,(char*)MEditType,
        DI_EDIT,24,3,25,3,0,0,0,0,type,
        DI_TEXT,5,4,0,0,0,0,0,0,(char*)MEditStart,
        DI_EDIT,24,4,29,4,0,0,0,0,start,
        DI_TEXT,3,5,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
        DI_BUTTON,0,6,0,0,0,0,DIF_CENTERGROUP,1,(char*)MOk,
        DI_BUTTON,0,6,0,0,0,0,DIF_CENTERGROUP,0,(char*)MCancel
      };
      struct FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
      initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
      
      int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                       -1,-1,38,9,
                                       NULL,
                                       dialogItems,
                                       sizeof(dialogItems)/sizeof(dialogItems[0]));
      if(askCode != 8) return TRUE;
      
      char *ptr = dialogItems[2].Data;
      for(int i = 0; i < 8; ++i) files[fNum].name[i] = ' ';
      
      for(int i = 0; i < 8; ++i)
      {
        if(*ptr > 0) files[fNum].name[i] = *ptr;
        if(*ptr == 0) break;
        ++ptr;
      }
      
      ptr = dialogItems[4].Data;
      files[fNum].type = 'C';
      if(*ptr > 0) files[fNum].type = *ptr;
      
      int result = str2int(dialogItems[6].Data, 5);
      if(result != -1) files[fNum].start = result;
      
      DWORD newCheckSum = calculateCheckSum((BYTE*)&files[fNum], sizeof(FileHdr));
      
      if(!openHostFile()) return TRUE;
      DWORD noBytesRead, noBytesWritten;
      
      SetFilePointer(hostFile, fNum*sizeof(FileHdr) + 8 + 1, NULL, FILE_BEGIN);
      WriteFile(hostFile, &files[fNum], sizeof(FileHdr), &noBytesWritten, NULL);
      SetFilePointer(hostFile, -4, NULL, FILE_END);
      
      DWORD checkSum;
      ReadFile(hostFile, &checkSum, 4, &noBytesRead, NULL);
      checkSum = checkSum - oldCheckSum + newCheckSum;
      SetFilePointer(hostFile, -4, NULL, FILE_END);
      WriteFile(hostFile, &checkSum, 4, &noBytesWritten, NULL);
      
      closeHostFile();
      startupInfo.Control(this,FCTL_UPDATEPANEL,(void *)1);
      startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL,(void *)1);

      return TRUE;
    }
  }
  return FALSE;
}
