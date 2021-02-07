#include <windows.h>
#include "plugin.hpp"

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

bool Manager::deleteFilesImpl (BYTE noItems)
{
  char tempDirName[300];
  if(GetTempPath(299, tempDirName) == 0) lstrcpy(tempDirName, ".");

  char tempFileName[300];
  if(GetTempFileName(tempDirName, "scl", 0, tempFileName) == 0) return false;
  
  if(!openHostFile()) return false;
  
  HANDLE tempFile = CreateFile(tempFileName,
                               GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS	,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
  if(tempFile == INVALID_HANDLE_VALUE)
  {
    errorMessage(getMsg(MCanNotCreateTmp));
    closeHostFile();
    return false;
  }
  
  DWORD checkSum       = writeSCLHeader(tempFile, noFiles - noItems);
  BYTE  noFilesWritten = 0;
  
  DWORD noBytesWritten;
  for(int i = 0; i < noFiles; ++i)
  {
    if(!pcFiles[i].deleted)
    {
      // пишем заголовок
      SetFilePointer(tempFile, noFilesWritten*sizeof(FileHdr)+8+1, NULL, FILE_BEGIN);
      WriteFile(tempFile, &files[i], sizeof(FileHdr), &noBytesWritten, NULL);
      checkSum += calculateCheckSum((BYTE*)&files[i], sizeof(FileHdr));
      
      checkSum += copyFile(i, tempFile);
      ++noFilesWritten;
    }
  }
  // пишем контрольную сумму
  WriteFile(tempFile, &checkSum, sizeof(checkSum), &noBytesWritten, NULL);
  
  CloseHandle(tempFile);
  closeHostFile();
  DeleteFile(hostFileName);
  MoveFile(tempFileName, hostFileName);
  op.reread = true;
  startupInfo.Control(this,FCTL_UPDATEANOTHERPANEL, (void *)1);
  op.reread = false;
  return true;
}

int Manager::deleteFiles(PluginPanelItem *panelItem, int noItems, int opMode)
{
  if(noItems == 0) return TRUE;
  if(isReadOnly)
  {
    char *msgItems[4];
    msgItems [0] = getMsg(MWarning);
    msgItems [1] = getMsg(MCanNotDelete);
    msgItems [2] = getMsg(MWriteProtected);
    msgItems [3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return FALSE;
  }

  readInfo();

  if((opMode & OPM_SILENT) == 0)
  {
    char *msgItems[5];
    msgItems[0] = getMsg(MDelete);
    msgItems[1] = getMsg(MDeleteAsk);
    msgItems[3] = getMsg(MDeleteButton);
    msgItems[4] = getMsg(MCancel);
    char msg[30];
    if(noItems == 1)
      wsprintf(msg, "%s", panelItem[0].FindData.cFileName);
    else
      wsprintf(msg, getMsg(MDelFiles), noItems);
    
    msgItems[2] = msg;
    if(messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 2)!=0)
      return FALSE;
    if(noItems > 1)
    {
      char *msgItems[5];
      msgItems[0] = getMsg(MDeleteFiles);
      msgItems[1] = getMsg(MDeleteAsk);
      msgItems[3] = getMsg(MAll);
      msgItems[4] = getMsg(MCancel);

      char msg[30];
      wsprintf(msg, getMsg(MDelFiles), noItems);
      msgItems[2] = msg;
      if(messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 2)!=0)
        return FALSE;
    }
  }
  
  // ищем файлы которые нужно удалить
  int noFoundFiles = 0;
  for(int i = 0; i < noFiles; ++i)
  {
    for(int j = 0; j < noItems; ++j)
      if(!lstrcmp(pcFiles[i].name, panelItem[j].FindData.cFileName))
      {
        pcFiles[i].deleted = true;
        ++noFoundFiles;
        break;
      }
  }
  if(noFoundFiles != noItems) return FALSE;
  return (deleteFilesImpl(noItems) ? TRUE : FALSE);
}
