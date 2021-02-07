#include <windows.h>
#include "plugin.hpp"

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;

int Manager::checkExistingFiles(FileHdr* hdrs,
                                FileHdr hdr,
                                int no_files,
                                int& allActions)
{
  // проверяем наличие файла с таким именем
  bool fileAlreadyExists = false;
  int fNum;
  for(fNum = 0; fNum < no_files; ++fNum)
    if(compareMemory(hdrs[fNum].name, hdr.name, 9))
    {
      fileAlreadyExists = true;
      break;
    }
  if(fileAlreadyExists)
  {
    if(allActions == 2) return 0;
    if(allActions == 0)
    {
      char *msgItems[13];
      msgItems[0] = getMsg(MWarning);
      msgItems[1] = getMsg(MAlreadyExists);
      msgItems[3] = "\x001";
      msgItems[4] = getMsg(MLine);
      msgItems[7] = "\x001";
      msgItems[8] = getMsg(MWrite);
      msgItems[9] = getMsg(MAll);
      msgItems[10] = getMsg(MSkip);
      msgItems[11] = getMsg(MSkipAll);
      msgItems[12] = getMsg(MCancel);
      
      BYTE name[13] = "            ";
      makeTrDosName(name, hdr, 12);
      msgItems[2] = name;
      
      char source[50];
      char dest[50];
      wsprintf(source, getMsg(MSource),
               (int)hdr.size,
               hdr.noSecs);
      wsprintf(dest,   getMsg(MDestination),
               (int)hdrs[fNum].size,
               hdrs[fNum].noSecs);
      
      msgItems[5] = source;
      msgItems[6] = dest;

      int askCode = messageBox(FMSG_WARNING | FMSG_DOWN,
                               msgItems,
                               sizeof(msgItems)/sizeof(msgItems[0]), 5);
      switch(askCode)
      {
        case -1:
        case 4:
                  return -1;
        case 3:
                  allActions = 2; //skipAll
        case 2:
                  return 0;
        case 1:
                  allActions = 1; //overwriteAll
      }
    }
  }
  return 1;
}

int Manager::putFiles(PluginPanelItem *panelItem, int noItems, int move, int opMode)
{
  if(noItems == 0) return 1;
  if(isReadOnly)
  {
    char *msgItems[4];
    msgItems [0] = getMsg(MWarning);
    msgItems [1] = getMsg(MCanNotCopy);
    msgItems [2] = getMsg(MWriteProtected);
    msgItems [3] = getMsg(MOk);
    messageBox(FMSG_WARNING, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 1);
    return FALSE;
  }

  char tempDirName[300];
  if(GetTempPath(299, tempDirName) == 0) lstrcpy(tempDirName, ".");

  char outFileName [300];
  char bodyFileName[300];
  
  if(GetTempFileName(tempDirName, "out",  0, outFileName)  == 0) return 0;
  if(GetTempFileName(tempDirName, "body", 0, bodyFileName) == 0) return 0;
  
  if(!openHostFile()) return 0;
  
  HANDLE bodyFile = CreateFile(bodyFileName,
                               GENERIC_WRITE | GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_DELETE_ON_CLOSE,
                               NULL);
  
  if(bodyFile == INVALID_HANDLE_VALUE)
  {
    closeHostFile();
    errorMessage(getMsg(MCanNotCreateTmp));
    return 0;
  }
  
  HANDLE outFile = CreateFile(outFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
  
  if(outFile == INVALID_HANDLE_VALUE)
  {
    CloseHandle(bodyFile);
    closeHostFile();
    errorMessage(getMsg(MCanNotCreateTmp));
    return 0;
  }
  
  DWORD   noBytesWritten, noBytesRead;
  FileHdr hdrs[255];

  DWORD noSecsInHostFile = 0;   // общее колличество секторов в host файле
  for(int i = 0; i < noFiles; ++i)
  {
    hdrs[i] = files[i];
    noSecsInHostFile += files[i].noSecs;
  }
  
  int   exitCode = 1;
  BYTE  totalFiles   = noFiles; // общее колличество файлов
  DWORD totalSectors = 0;       // общее колличество секторов в копируемых файлах

  int   allActions = 0;         // данные о нажатии кнопок write_all и skip_all

  HANDLE file;
  HANDLE screen = startupInfo.SaveScreen(0,0,-1,-1);
  for(int iNum = 0; iNum < noItems; ++iNum)
  {
    char *msgItems[2];
    msgItems[0] = getMsg(MCopyingFile);
    msgItems[1] = pointToName(panelItem[iNum].FindData.cFileName);
    if((opMode & OPM_SILENT) == 0)
      messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);

    bool isFullFileCopied = true;
    
    file = CreateFile(panelItem[iNum].FindData.cFileName,
                      GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);
    
    if(file == INVALID_HANDLE_VALUE)
    {
      char msg[300];
      wsprintf(msg, getMsg(MCanNotOpen), panelItem[iNum].FindData.cFileName);
      errorMessage(msg);
      exitCode = 0;
      continue;
    }
    
    FileType file_type = detectFileType(file);
    if(file_type == FMT_HOBETA)
    {
      if(isSCLFull(totalFiles, 1, file))
      {
        exitCode = 0;
        break;
      }
      HoHdr ho_hdr;
      ReadFile(file, &ho_hdr, sizeof(HoHdr), &noBytesRead, 0);

      FileHdr hdr;
      CopyMemory(hdr.name, ho_hdr.name, 8);
      hdr.type     = ho_hdr.type;
      hdr.start    = ho_hdr.start;
      hdr.size     = ho_hdr.size;
      hdr.noSecs   = ho_hdr.noSecs;
   
      int r = checkExistingFiles(hdrs, hdr, totalFiles, allActions);
      if(r == -1)   // нажат Cancel
      {
        exitCode = -1;
        break;
      }
      if(r == 0)    // нажат Skip/Skip all
      {
        CloseHandle(file);
        exitCode = 0;
        continue;
      }
      hdrs[totalFiles++] = hdr;
      
      copySectors(bodyFile, file, hdr.noSecs);

      totalSectors += hdr.noSecs;
    }
    if(file_type == FMT_SCL)
    {
      SetFilePointer(file, 8, NULL, FILE_BEGIN);
      BYTE no_files;
      ReadFile(file, &no_files, 1, &noBytesRead, 0);
      
      if(isSCLFull(totalFiles, no_files, file))
      {
        exitCode = 0;
        break;
      }

      int   totalSecs  = 0;
      DWORD fileOffset = no_files*sizeof(FileHdr) + 8 + 1;
      
      for(int fNum = 0; fNum < no_files; ++fNum)
      {
        FileHdr hdr;
        SetFilePointer(file, fNum*sizeof(FileHdr) + 8 + 1, NULL, FILE_BEGIN);
        ReadFile(file, &hdr, sizeof(FileHdr), &noBytesRead, NULL);

        int r = checkExistingFiles(hdrs, hdr, totalFiles, allActions);
        if(r == -1)   // нажат Cancel
        {
          exitCode = -1;
          break;
        }
        
        if(r == 0)  // нажат Skip/Skip all
        {
          isFullFileCopied = false;
          fileOffset += hdr.noSecs*sectorSize;
          exitCode = 0;
          continue;
        }

        hdrs[totalFiles++] = hdr;

        SetFilePointer(file, fileOffset, NULL, FILE_BEGIN);
        copySectors(bodyFile, file, hdr.noSecs);
        fileOffset += hdr.noSecs*sectorSize;
        
        totalSecs += hdr.noSecs;
      }
      totalSectors += totalSecs;
      if(exitCode == -1) break;
    }
    if(file_type == FMT_PLAIN)
    {
      DWORD fileSize  = panelItem[iNum].FindData.nFileSizeLow;
      int   sizeInSec = fileSize/sectorSize  + (fileSize%sectorSize ? 1 : 0);
      int   no_files  = sizeInSec/255 + (sizeInSec%255 ? 1: 0);
      if(!no_files) no_files = 1;
      
      if(isSCLFull(totalFiles, no_files, file))
      {
        exitCode = 0;
        break;
      }
      
      // формируем имя
      char name[8]  = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
      if(no_files > 1) FillMemory(name, 8, '_');
      
      BYTE* ptr = pointToName(panelItem[iNum].FindData.cFileName);
      int pos = 0;
      while(*ptr && *ptr != '.')
      {
        name[pos++] = (*ptr < 0x80 && *ptr != 0x01) ? *ptr : '_';
        ++ptr;
        if(pos == 8) break;
      }
      
      // формируем расширение
      char ext[3]   = {'C', 0, 0};
      ptr = pointToExt(panelItem[iNum].FindData.cFileName);
      pos = 0;
      if(ptr)
      {
        while(*ptr)
        {
          ext[pos++] = (*ptr < 0x80) ? *ptr : '_';
          ++ptr;
          if(pos == 3) break;
        }
      }
      DWORD totalSecs = 0;
      for(int fNum = 0; fNum < no_files; ++fNum)
      {
        FileHdr hdr;
        
        if(no_files > 1)
        {
          char num[5];
          wsprintf(num, "_%03d", fNum);
          CopyMemory(name + 4, num, 4);
        }
        
        CopyMemory(hdr.name, name, 8);
        CopyMemory(&hdr.type, ext,  3);
        if(sizeInSec > 255)
        {
          hdr.noSecs = 255;
          hdr.size   = 255 * sectorSize;
          
          sizeInSec -= 255;
          fileSize  -= 255 * sectorSize;
        }
        else
        {
          // последний кусок
          hdr.noSecs = sizeInSec;
          hdr.size   = fileSize;
        }

        int r = checkExistingFiles(hdrs, hdr, totalFiles, allActions);
        if(r == -1)   // нажат Cancel
        {
          exitCode = -1;
          break;
        }
        
        if(r == 0)  // нажат Skip/Skip all
        {
          isFullFileCopied = false;
          exitCode = 0;
          continue;
        }
        
        hdrs[totalFiles++] = hdr;
        
        SetFilePointer(file, fNum*255*sectorSize, NULL, FILE_BEGIN);
        copySectors(bodyFile, file, hdr.noSecs);
        
        totalSecs += hdr.noSecs;
      }
      if(exitCode == -1) break;
      totalSectors += totalSecs;
    }
    CloseHandle(file);
    file = INVALID_HANDLE_VALUE;
    if(isFullFileCopied)
    {
      panelItem[iNum].Flags ^= PPIF_SELECTED;
      if(move) DeleteFile(panelItem[iNum].FindData.cFileName);
    }
  }
  // если вышли из цикла по break
  if(file != INVALID_HANDLE_VALUE) CloseHandle(file);

  char *msgItems[2];
  msgItems[0] = getMsg(MCopy);
  msgItems[1] = getMsg(MUpdateFiles);
  if((opMode & OPM_SILENT) == 0)
    messageBox(0, msgItems, sizeof(msgItems)/sizeof(msgItems[0]), 0);

  BYTE  signature[] = {'S', 'I', 'N', 'C', 'L', 'A', 'I', 'R' };
  DWORD checkSum = 0x255;
  
  WriteFile(outFile, signature, sizeof(signature), &noBytesWritten, NULL);
  WriteFile(outFile, &totalFiles, 1, &noBytesWritten, NULL);
  checkSum += totalFiles;
  for(int i = 0; i < totalFiles; ++i)
  {
    WriteFile(outFile, &hdrs[i], sizeof(FileHdr), &noBytesWritten, NULL);
    checkSum += calculateCheckSum((BYTE*)&hdrs[i], sizeof(FileHdr));
  }

  DWORD firstFileOffset = noFiles*sizeof(FileHdr) + 8 + 1;
  SetFilePointer(hostFile, firstFileOffset, NULL, FILE_BEGIN);
  checkSum += copySectors(outFile, hostFile, noSecsInHostFile);
  
  SetFilePointer(bodyFile, 0, NULL, FILE_BEGIN);
  checkSum += copySectors(outFile, bodyFile, totalSectors);
  
  WriteFile(outFile, &checkSum, 4, &noBytesWritten, NULL);
  
  CloseHandle(bodyFile);
  CloseHandle(outFile);
  closeHostFile();
  
  DeleteFile(hostFileName);
  MoveFile(outFileName, hostFileName);

  startupInfo.RestoreScreen(screen);
  return exitCode;
}
