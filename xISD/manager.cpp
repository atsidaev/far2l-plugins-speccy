#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "manager.hpp"
#include "tools.hpp"
#include "iSDOS.hpp"
#include "iSDOS_tools.hpp"
#include "lang.hpp"

#include <string>
#include "widestring.hpp"

extern PluginStartupInfo startupInfo;
extern Options           op;

Manager::Manager(char* fileName, FmtPlugin* fmtPlugin)
{
  strcpy(hostFileName, fileName);
  filer = fmtPlugin;
  img = filer->openSubPlugin(fileName);
  *curDir    = 0;
  memset(&lastModifed, 0, sizeof(WIN32_FIND_DATA));

  filer->reload(img);
  openHostFile();
  // читаем информацию о диске
  u8 buf[blockSize];
  readBlock(0, buf);
  memcpy(&dsk, buf, sizeof(DiskHdr));

  // читаем главный каталог
  readFolder(dsk.mainDir1stBlock, files);

  for(int fNum = 1; fNum < files[0].dir.totalFiles; ++fNum)
  {
    if(files[fNum].attr & FLAG_EXIST && !isDir(files[fNum]))
    {
      if(compareMemory(files[fNum].name, "device  sys", 11))
      {
        memcpy(&device_sys_hdr, &files[fNum], sizeof(UniHdr));
        u32 size = getSize(device_sys_hdr);
        
        int noBlocks = size/blockSize + ((size%blockSize) ? 1 : 0);
        device_sys = (u8*)malloc(blockSize*noBlocks);
        read_device_sys();
        break;
      }
    }
  }

  closeHostFile();
}

Manager::~Manager()
{
  free(device_sys);
  filer->closeSubPlugin(img);
}

void Manager::getOpenPluginInfo(OpenPluginInfo *info)
{
  info->StructSize = sizeof(*info);
  info->Flags      = OPIF_USEFILTER | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_SHOWPRESERVECASE;
  info->HostFile   = hostFileName;
  info->CurDir     = curDir;
  info->Format     = "iS-DOS";

  static char panelTitle[300];

  *panelTitle = 0;
  strcat(panelTitle, " iS-DOS:");
  strcat(panelTitle, pointToName(hostFileName));
  strcat(panelTitle, curDir);
  strcat(panelTitle, " ");
  info->PanelTitle = panelTitle;

  static PanelMode mode[5];
  static char* columnTitles[5] = { 0,0,0,0,0 };
  if (columnTitles[0] == 0)
  {
    for (int i = 0; i < 5; i++)
      columnTitles[i] = new char[16];
  }

  char* p = op.columnTitles;
  int n = 0;
  int i = 0;
  while(*p)
  {
    if(*p != ',')
      columnTitles[n][i++] = *p++;
    else
    {
      ++p;
      columnTitles[n++][i] = 0;
      i = 0;
      if(n == 5) break;
    }
  }
  
  mode[4].ColumnTypes        = op.columnTypes;
  mode[4].ColumnWidths       = op.columnWidths;
  mode[4].ColumnTitles       = columnTitles;
  mode[4].StatusColumnTypes  = "NR,SC,DMBM";
  mode[4].StatusColumnWidths = "0,8,0";
  mode[4].AlignExtensions    = TRUE;
  mode[4].FullScreen         = FALSE;

  info->PanelModesArray  = mode;
  info->PanelModesNumber = sizeof(mode)/sizeof(mode[0]);
  info->StartPanelMode   = '0' + op.defaultPanelMode;
  if(readInfo())
  {
    static InfoPanelLine infoLines[4];
    int noChars;
    if(dsk.signatureChic[0] == 'D' && dsk.signatureChic[1] == 'S' && dsk.signatureChic[2] == 'K')
    {
      noChars = 11;
      strcpy(infoLines[0].Text, getMsg(MChic));
    }
    else
    {
      noChars = 8;
      strcpy(infoLines[0].Text, getMsg(MClassic));
    }
    infoLines[0].Separator = TRUE;
    
    strcpy(infoLines[1].Text, getMsg(MDiskTitle));
    while(noChars--)
      if(dsk.title[noChars] != ' ' && dsk.title[noChars] != 0) break;
    
    strncpy(infoLines[1].Data, (const char*)dsk.title, noChars+2);
    infoLines[1].Separator = FALSE;
    
    strcpy(infoLines[2].Text, getMsg(MTotalBlocks));
    sprintf(infoLines[2].Data, "%d", dsk.noBlocks);
    infoLines[2].Separator = FALSE;
    
    strcpy(infoLines[3].Text, getMsg(MFreeBlocks));
    sprintf(infoLines[3].Data, "%d", noFreeBlocks);
    infoLines[3].Separator = FALSE;
    info->InfoLines = infoLines;
    info->InfoLinesNumber = sizeof(infoLines)/sizeof(infoLines[0]);
  }

}

int Manager::getFindData(PluginPanelItem **pPanelItem, int *pNoItems, int opMode)
{
  *pNoItems   = 0;
  *pPanelItem = NULL;

  if(!readInfo()) return FALSE;
  
  PluginPanelItem* item = (PluginPanelItem*)malloc(files[0].dir.noFiles*sizeof(PluginPanelItem));
  memset(item, 0, (files[0].dir.noFiles)*sizeof(PluginPanelItem));
  
  int inx = 0;
  
  for(int fNum = 1; fNum < files[0].dir.totalFiles; ++fNum)
  {
    if(files[fNum].attr & FLAG_EXIST)
    {
      item[inx].CustomColumnNumber = 3;
      item[inx].CustomColumnData = (LPSTR*)malloc(sizeof(LPSTR)*item[inx].CustomColumnNumber);
      
      item[inx].CustomColumnData[0] = (char*)malloc(5);
      item[inx].CustomColumnData[1] = (char*)malloc(6);
      item[inx].CustomColumnData[2] = (char*)malloc(6);

      FILETIME ft;
      memset(&ft, 0, sizeof(ft));
      if(isDir(files[fNum]))
      {
        DosDateTimeToFileTime(files[fNum].date, 0, &ft);
        item[inx].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        if(files[fNum].attr & FLAG_SOLID)
          sprintf(item[inx].CustomColumnData[1], "%5d", files[fNum].dir.firstBlock);
        else
          sprintf(item[inx].CustomColumnData[1], "%5d", files[fNum].dir.descr1stBlock);
        item[inx].CustomColumnData[2][0] = 0;
      }
      else
      {
        item[inx].FindData.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        item[inx].FindData.nFileSize     = getSize(files[fNum]);
        sprintf(item[inx].CustomColumnData[1], "%5d", files[fNum].file.firstBlock);
        sprintf(item[inx].CustomColumnData[2], "%5d", files[fNum].file.loadAddr);
        DosDateTimeToFileTime(files[fNum].date, files[fNum].file.time, &ft);
      }
      if(files[fNum].attr & FLAG_HIDDEN)
        item[inx].FindData.dwFileAttributes |= FILE_ATTRIBUTE_HIDDEN;
      
      LocalFileTimeToFileTime(&ft, &ft);
      makeName(files[fNum], (u8*)item[inx].FindData.cFileName);

      item[inx].FindData.ftLastWriteTime = ft;
      
      strcpy(item[inx].CustomColumnData[0], "....");
      if(!(files[fNum].attr & FLAG_SOLID))       item[inx].CustomColumnData[0][0] = 'S';
      if(files[fNum].attr & FLAG_READ_PROTECT)   item[inx].CustomColumnData[0][1] = 'R';
      if(files[fNum].attr & FLAG_WRITE_PROTECT)  item[inx].CustomColumnData[0][2] = 'W';
      if(files[fNum].attr & FLAG_DELETE_PROTECT) item[inx].CustomColumnData[0][3] = 'D';
      ++inx;
    }
  }
  *pNoItems   = inx;
  *pPanelItem = item;
  return TRUE;
}

void Manager::freeFindData(PluginPanelItem *panelItem, int  noItems)
{
  for (int i = 0; i < noItems; i++)
  {
    free(panelItem[i].CustomColumnData[0]);
    free(panelItem[i].CustomColumnData[1]);
    free(panelItem[i].CustomColumnData[2]);
    free(panelItem[i].CustomColumnData);
  }
  free(panelItem);
}

int Manager::setDirectory(char* dirName, int opMode)
{
  u8 buf[blockSize];
  if(*dirName == '\\')
    if(*(++dirName) == 0)
    {
      *curDir = 0;
      if(!openHostFile()) return FALSE;
      readFolder(dsk.mainDir1stBlock, files);
      closeHostFile();
      return TRUE;
    }
  if(!strcmp(dirName, ".."))
  {
    if(*curDir == 0) return FALSE;

    if(!openHostFile()) return FALSE;
    readFolder(files[0].dir.parentDir1stBlock, files);
    closeHostFile();

    int i = strlen(curDir) - 2;
    while(i != -1 && curDir[i] != '\\') --i;
    curDir[i] = 0;
  }
  else
  {
    int fNum = 1;
    u8 name[2*(8+3+2)];
    for(;fNum < files[0].dir.totalFiles; ++fNum)
    {
      if(!(files[fNum].attr & FLAG_EXIST)) continue;
      if(!isDir(files[fNum])) continue;
      makeName(files[fNum], name);
      if(!strcmp((const char*)name, dirName)) break;
    }
    if(fNum == files[0].dir.totalFiles) return FALSE;

    if(!openHostFile()) return FALSE;
    readFolder(files[fNum], files);
    closeHostFile();
    
    strcat(curDir, "\\");
    strcat(curDir, dirName);
  }
  return TRUE;
}
