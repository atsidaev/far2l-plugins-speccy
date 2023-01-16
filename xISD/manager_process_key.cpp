#include <windows.h>

#include "manager.hpp"
#include "tools.hpp"
#include "iSDOS.hpp"
#include "iSDOS_tools.hpp"
#include "lang.hpp"

#include "../shared/widestring.hpp"

extern PluginStartupInfo startupInfo;

int str2int(char* p, int len)
{
  int result = -1;
  while(*p == ' ') ++p;
  
  for(int i = 0; i < len; ++i)
  {
    u8 digit = 0xFF;
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
  if(key == VK_RETURN && !(controlState & PKF_CONTROL))
  {
    PanelInfo info;
    startupInfo.Control(this,FCTL_GETPANELINFO,&info);
    char *fName = info.PanelItems[info.CurrentItem].FindData.cFileName;
    int fNum = findFile(files, fName);
    if (strncmp((char*)files[fNum].ext, "com", 3) == 0) return TRUE;
    if (strncmp((char*)files[fNum].ext, "bat", 3) == 0) return TRUE;
    if (strncmp((char*)files[fNum].ext, "exe", 3) == 0) return TRUE;
    if (strncmp((char*)files[fNum].ext, "cmd", 3) == 0) return TRUE;
  }

  if((controlState & PKF_CONTROL) && key == 0x41)
  {
    HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
    PanelInfo info;
    startupInfo.Control(this,FCTL_GETPANELINFO,&info);
    startupInfo.RestoreScreen(screen);
    
    if(info.ItemsNumber > 0 && info.CurrentItem > 0)
    {
      char *fName = info.PanelItems[info.CurrentItem].FindData.cFileName;
      int fNum = findFile(files, fName);
      if(!fNum) return TRUE;

      char name[8+3+2] = "";
      makeName(files[fNum], (u8*)name);

      int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
      char day[3], month[3], year[5];
      SYSTEMTIME st = makeDate(files[fNum]);
      sprintf(day,  "%02d",st.wDay);
      sprintf(month,"%02d",st.wMonth);
      sprintf(year, "%04d",st.wYear);

      if (isDir(files[fNum]))
      {
        InitDialogItem items[]=
        {
          DI_DOUBLEBOX,3,1,41,14,0,0,0,0,(char *)MAttributes,
          DI_TEXT,5,2,0,0,0,0,DIF_CENTERGROUP,0,(char *)MAttrsFor,
          DI_TEXT,5,3,0,0,1,0,DIF_CENTERGROUP,0,name,
          DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_CHECKBOX,5,5,0,0,TRUE,0,0,0,(char *)MReadProtect,
          DI_CHECKBOX,5,6,0,0,0,0,0,0,(char *)MWriteProtect,
          DI_CHECKBOX,5,7,0,0,0,0,0,0,(char *)MDeleteProtect,
          DI_CHECKBOX,5,8,0,0,0,0,0,0,(char *)MHidden,
          DI_TEXT,3,9,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_TEXT,10,10,0,0,0,0,0,0,(char *)MDDMMYYYY,
          DI_TEXT,5,11,0,0,0,0,0,0,(char *)MDate,
          DI_FIXEDIT,10,11,11,13,0,0,0,0,day,
          DI_TEXT,12,11,0,0,0,0,0,0,".",
          DI_FIXEDIT,13,11,14,13,0,0,0,0,month,
          DI_TEXT,15,11,0,0,0,0,0,0,".",
          DI_FIXEDIT,16,11,19,13,0,0,0,0,year,

          DI_TEXT,     3, 12,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_BUTTON,   0, 13,0,0,0,0,DIF_CENTERGROUP,1,(char *)MOk,
          DI_BUTTON,   0, 13,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
        };
        FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
        initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
        
        dialogItems[4].Selected = files[fNum].attr & FLAG_READ_PROTECT;
        dialogItems[5].Selected = files[fNum].attr & FLAG_WRITE_PROTECT;
        dialogItems[6].Selected = files[fNum].attr & FLAG_DELETE_PROTECT;
        dialogItems[7].Selected = files[fNum].attr & FLAG_HIDDEN;
   
        int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                         -1,-1,45,16,
                                         NULL,
                                         dialogItems,
                                         sizeof(dialogItems)/sizeof(dialogItems[0]));
        if(askCode != 17) return TRUE;
   
        files[fNum].attr &= 0xFF & ~(FLAG_READ_PROTECT | FLAG_WRITE_PROTECT | FLAG_DELETE_PROTECT | FLAG_HIDDEN);
        if (dialogItems[4].Selected) files[fNum].attr |= FLAG_READ_PROTECT;
        if (dialogItems[5].Selected) files[fNum].attr |= FLAG_WRITE_PROTECT;
        if (dialogItems[6].Selected) files[fNum].attr |= FLAG_DELETE_PROTECT;
        if (dialogItems[7].Selected) files[fNum].attr |= FLAG_HIDDEN;
   
        int result = str2int(dialogItems[13].Data, 2);
        if((result <= 12)&&(result > 0)) st.wMonth = result;
        result = str2int(dialogItems[15].Data, 4);
        if(result != -1)
        {
          if(result < 80) st.wYear = result + 2000;
          if((result >= 80)&&(result < 100)) st.wYear = result + 1900;
          if((result >= 1980)&&(result < 2100)) st.wYear = result;
        }
        result = str2int(dialogItems[11].Data, 2);
        if(result >0)
        {
          if(result <= days[st.wMonth-1]) st.wDay = result;
          if((result == 29)&&(st.wMonth==2)&&(!(st.wYear%4))) st.wDay = result;
        }
      }
      else
      {
        char stAdr[6], hour[3], minute[3], second[3];
        sprintf(hour,  "%02d",st.wHour);
        sprintf(minute,"%02d",st.wMinute);
        sprintf(second,"%02d",st.wSecond);
        sprintf(stAdr,"%d",files[fNum].file.loadAddr);

        InitDialogItem items[]=
        {
          DI_DOUBLEBOX,3,1,41,16,0,0,0,0,(char *)MAttributes,
          DI_TEXT,5,2,0,0,0,0,DIF_CENTERGROUP,0,(char *)MAttrsFor,
          DI_TEXT,5,3,0,0,1,0,DIF_CENTERGROUP,0,name,
          DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_CHECKBOX,5,5,0,0,TRUE,0,0,0,(char *)MReadProtect,
          DI_CHECKBOX,5,6,0,0,0,0,0,0,(char *)MWriteProtect,
          DI_CHECKBOX,5,7,0,0,0,0,0,0,(char *)MDeleteProtect,
          DI_CHECKBOX,5,8,0,0,0,0,0,0,(char *)MHidden,
          DI_TEXT,3,9,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_TEXT,5,10,0,0,0,0,0,0,(char *)MStartAdr,
          DI_EDIT,34,10,39,10,0,0,0,0,stAdr,
          DI_TEXT,3,11,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_TEXT,10,12,0,0,0,0,0,0,(char *)MDDMMYYYY,
          DI_TEXT,32,12,0,0,0,0,0,0,(char *)Mhhmmss,
          DI_TEXT,5,13,0,0,0,0,0,0,(char *)MDate,
          DI_FIXEDIT,10,13,11,13,0,0,0,0,day,
          DI_TEXT,12,13,0,0,0,0,0,0,".",
          DI_FIXEDIT,13,13,14,13,0,0,0,0,month,
          DI_TEXT,15,13,0,0,0,0,0,0,".",
          DI_FIXEDIT,16,13,19,13,0,0,0,0,year,
          DI_TEXT,26,13,0,0,0,0,0,0,(char *)MTime,
          DI_FIXEDIT,32,13,33,13,0,0,0,0,hour,
          DI_TEXT,34,13,0,0,0,0,0,0,":",
          DI_FIXEDIT,35,13,36,13,0,0,0,0,minute,
          DI_TEXT,37,13,0,0,0,0,0,0,":",
          DI_FIXEDIT,38,13,39,13,0,0,0,0,second,
          DI_TEXT,3,14,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
          DI_BUTTON,0,15,0,0,0,0,DIF_CENTERGROUP,1,(char *)MOk,
          DI_BUTTON,0,15,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
        };
        FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
        initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
        
        dialogItems[4].Selected = files[fNum].attr & FLAG_READ_PROTECT;
        dialogItems[5].Selected = files[fNum].attr & FLAG_WRITE_PROTECT;
        dialogItems[6].Selected = files[fNum].attr & FLAG_DELETE_PROTECT;
        dialogItems[7].Selected = files[fNum].attr & FLAG_HIDDEN;
   
        int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                         -1,-1,45,18,
                                         NULL,
                                         dialogItems,
                                         sizeof(dialogItems)/sizeof(dialogItems[0]));
        if(askCode != 27) return TRUE;
        if(files[fNum].attr == 0xFF && files[fNum].file.systemFlag == 0xFF) return TRUE;
   
        files[fNum].attr &= 0xFF & ~(FLAG_READ_PROTECT | FLAG_WRITE_PROTECT | FLAG_DELETE_PROTECT | FLAG_HIDDEN);
        if (dialogItems[4].Selected) files[fNum].attr |= FLAG_READ_PROTECT;
        if (dialogItems[5].Selected) files[fNum].attr |= FLAG_WRITE_PROTECT;
        if (dialogItems[6].Selected) files[fNum].attr |= FLAG_DELETE_PROTECT;
        if (dialogItems[7].Selected) files[fNum].attr |= FLAG_HIDDEN;
   
        int result = str2int(dialogItems[10].Data, 5);
        if((result != -1)&&(result < 65536)) files[fNum].file.loadAddr = result;

        result = str2int(dialogItems[17].Data, 2);
        if((result <= 12)&&(result > 0)) st.wMonth = result;
        result = str2int(dialogItems[19].Data, 4);
        if(result != -1)
        {
          if(result < 80) st.wYear = result + 2000;
          if((result >= 80)&&(result < 100)) st.wYear = result + 1900;
          if((result >= 1980)&&(result < 2100)) st.wYear = result;
        }
        result = str2int(dialogItems[15].Data, 2);
        if(result >0)
        {
          if(result <= days[st.wMonth-1]) st.wDay = result;
          if((result == 29)&&(st.wMonth==2)&&(!(st.wYear%4))) st.wDay = result;
        }

        result = str2int(dialogItems[21].Data, 2);
        if((result < 24)&&(result != -1)) st.wHour = result;
        result = str2int(dialogItems[23].Data, 2);
        if((result < 60)&&(result != -1)) st.wMinute = result;
        result = str2int(dialogItems[25].Data, 2);
        if((result < 60)&&(result != -1)) st.wSecond = result;
      }

      if(st.wDay > days[st.wMonth-1])
        if ((st.wDay > 29)&&(st.wMonth==2)&&(!(st.wYear%4))) st.wDay = 29;
        else st.wDay = days[st.wMonth-1];

      setDate(files[fNum],st);
      if(!openHostFile()) return TRUE;
      writeFolder(files);
      closeHostFile();
      startupInfo.Control(this,FCTL_UPDATEPANEL,(void *)1);        
      startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
      return TRUE;
    }
  return FALSE;
  }

  if((controlState & PKF_SHIFT) && key == VK_F6)
  {
    HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
    PanelInfo info;
    startupInfo.Control(this,FCTL_GETPANELINFO,&info);
    startupInfo.RestoreScreen(screen);
    
    if(info.ItemsNumber > 0 && info.CurrentItem > 0)
    {
      char *fName = info.PanelItems[info.CurrentItem].FindData.cFileName;
      int fNum = findFile(files, fName);
      if(!fNum) return TRUE;

      char name[8+3+2] = "";
      makeName(files[fNum], (u8*)name);

      char buf[100];
      sprintf(buf, getMsg(MRenameTo), name);
      InitDialogItem items[]=
      {
        DI_DOUBLEBOX,3,1,50,6,0,0,0,0,(char *)MRename,
        DI_TEXT,5,2,0,0,0,0,0,0,buf,
        DI_EDIT,5,3,48,3,1,0,0,0,name,

        DI_TEXT,3,4,0,0,0,0,DIF_BOXCOLOR|DIF_SEPARATOR,0,"",
        DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,1,(char *)MOk,
        DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
      };
      FarDialogItem dialogItems[sizeof(items)/sizeof(items[0])];
      initDialogItems(items, dialogItems, sizeof(items)/sizeof(items[0]));
      
      int askCode = startupInfo.Dialog(startupInfo.ModuleNumber,
                                       -1,-1,54,8,
                                       NULL,
                                       dialogItems,
                                       sizeof(dialogItems)/sizeof(dialogItems[0]));
      if(askCode != 4) return TRUE;

      if(isDir(files[fNum]))
        makeCompatibleFolderName(name, dialogItems[2].Data);
      else
      {
        if(files[fNum].attr == 0xFF && files[fNum].file.systemFlag == 0xFF)
          return TRUE;
        makeCompatibleFileName(name, dialogItems[2].Data);
      }

      char *p = name;

      if(*p == 0) return TRUE;
      memset(&files[fNum], ' ', 8+3);

      for(int i = 0; i < 8; ++i)
      {
        if(*p == 0 || *p =='.') break;
        files[fNum].name[i] = *p;
        ++p;
      }
      
      p = (char*)pointToExt(name);
      
      if(p)
      {
        for(int i = 0; i < 3; ++i)
        {
          if(*p == 0) break;
          files[fNum].ext[i] = *p;
          ++p;
        }
      }
      if(!openHostFile()) return TRUE;
      writeFolder(files);
      closeHostFile();
      startupInfo.Control(this,FCTL_UPDATEPANEL,(void *)1);        
      startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
      return TRUE;
    }
  }
  return FALSE;
}
