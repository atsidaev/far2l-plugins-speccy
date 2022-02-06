#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "manager.hpp"
#include "detector.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

#include "widestring.hpp"

extern PluginStartupInfo startupInfo;
extern Detector*         detector;
extern Options           op;

Manager::Manager(char* fileName, FmtPlugin *fmtPlugin)
{
  strcpy(hostFileName, fileName);
  fmt = fmtPlugin;
  noFiles      = 0;
  noDelFiles   = 0;
  curFolderNum = 0;
  *curFolder   = 0;
  
  img = fmt->init(hostFileName);
  memset(&lastModifed, 0, sizeof(WIN32_FIND_DATA));

  memset(fileMap,   0, sizeof(fileMap));
  memset(folderMap, 0, sizeof(folderMap));
}

Manager::~Manager()
{
  fmt->cleanup(img);
}

void Manager::getOpenPluginInfo(OpenPluginInfo* info)
{
  info->StructSize    = sizeof(*info);
  info->Flags         = OPIF_USEFILTER | OPIF_USEHIGHLIGHTING | OPIF_SHOWNAMESONLY | OPIF_SHOWPRESERVECASE;
  info->HostFile      = hostFileName;
  info->CurDir        = curFolder;
  info->Format        = "TR-DOS disk";
  info->StartSortMode = SM_UNSORTED;
  
  static char panelTitle[260];
  panelTitle[0] = 0;
  strcat(panelTitle, " xTRD:");
  strcat(panelTitle, pointToName(hostFileName));
  if(op.useDS) strcat(panelTitle, curFolder);
  strcat(panelTitle, " ");
  info->PanelTitle = panelTitle;

  static PanelMode mode[10];
  static char*     columnTitles3[4];
  columnTitles3[0] = getMsg(MName);
  columnTitles3[1] = getMsg(MSectorSize);
  columnTitles3[2] = getMsg(MName);
  columnTitles3[3] = getMsg(MSectorSize);

  static char*     columnTitles4[6];
  columnTitles4[0] = getMsg(MName);
  columnTitles4[1] = getMsg(MSize);
  columnTitles4[2] = getMsg(MStart);
  columnTitles4[3] = getMsg(MSectorSize);
  columnTitles4[4] = getMsg(MTrack);
  columnTitles4[5] = getMsg(MSector);

  static char*     columnTitles5[2];
  columnTitles5[0] = getMsg(MName);
  columnTitles5[1] = getMsg(MFormat);

  static char*     columnTitles6[2];
  columnTitles6[0] = getMsg(MName);
  columnTitles6[1] = getMsg(MComment);
  
  // C0 - trdos name
  // C1 - trdos 'start'
  // C2 - размер в секторах
  // С3 - дорожка
  // C4 - сектор
  // C5 - описание типа
  // C6 - комментарий к файлу
  
  mode[3].ColumnTypes="N,C2,N,C2";
  mode[3].ColumnWidths="0,3,0,3";
  mode[3].ColumnTitles=columnTitles3;
  mode[3].StatusColumnTypes="C0,S,C2";
  mode[3].StatusColumnWidths="0,6,3";
  mode[3].AlignExtensions=TRUE;
  mode[3].FullScreen=FALSE;
  
  mode[4].ColumnTypes="C0,S,C1,C2,C3,C4";
  mode[4].ColumnWidths="0,6,5,3,3,3";
  mode[4].StatusColumnTypes="N,S,C2";
  mode[4].StatusColumnWidths="0,6,3";
  mode[4].ColumnTitles=columnTitles4;
  mode[4].FullScreen=FALSE;
  
  mode[5].ColumnTypes="C0,C5";
  mode[5].ColumnWidths="12,0";
  mode[5].StatusColumnTypes="N,S,C2";
  mode[5].StatusColumnWidths="0,6,3";
  mode[5].ColumnTitles=columnTitles5;
  mode[5].FullScreen=FALSE;

  mode[6].ColumnTypes="C0,C6";
  mode[6].ColumnWidths="12,0";
  mode[6].StatusColumnTypes="N,S,C2";
  mode[6].StatusColumnWidths="0,6,3";
  mode[6].ColumnTitles=columnTitles6;
  mode[6].FullScreen=FALSE;
  
  info->PanelModesArray = mode;
  info->PanelModesNumber = sizeof(mode)/sizeof(mode[0]);
  info->StartPanelMode = '0' + op.defaultPanelMode;
  if(readInfo())
  {
    static InfoPanelLine infoLines[14];
    strcpy(infoLines[0].Text, getMsg(MDiskInfo));
    infoLines[0].Separator = TRUE;
    strcpy(infoLines[1].Text, getMsg(MImageType));
    strcpy(infoLines[1].Data, fmt->getFormatName());
    
    infoLines[1].Separator = FALSE;
    strcpy(infoLines[2].Text, getMsg(MDiskTitle));
    int noChars = 11;
    while(noChars--)
      if(diskInfo.title[noChars] != ' ' && diskInfo.title[noChars] != 0) break;
      
    strncpy(infoLines[2].Data, diskInfo.title, noChars+2);
    infoLines[2].Separator = FALSE;

    strcpy(infoLines[3].Text, getMsg(MDiskType));
    switch(diskInfo.type)
    {
      case 0x16:
                strcpy(infoLines[3].Data, getMsg(M2SDD));
                break;
      case 0x17:
                strcpy(infoLines[3].Data, getMsg(M2SSD));
                break;
      case 0x18:
                strcpy(infoLines[3].Data, getMsg(M1SDD));
                break;
      case 0x19:
                strcpy(infoLines[3].Data, getMsg(M1SSD));
                break;
      default:
                strcpy(infoLines[3].Data, getMsg(MDiskUnknown));
                break;
    }
    infoLines[3].Separator = FALSE;

    strcpy(infoLines[4].Text, getMsg(MWriteProtection));
    if(fmt->isProtected(img))
      strcpy(infoLines[4].Data, getMsg(MOn));
    else
      strcpy(infoLines[4].Data, getMsg(MOff));
    infoLines[4].Separator = FALSE;
    
    strcpy(infoLines[5].Text, getMsg(MFilesInfo));
    infoLines[5].Separator = TRUE;
    
    strcpy(infoLines[6].Text, getMsg(MFiles));
    sprintf(infoLines[6].Data, "%d", diskInfo.noFiles);
    infoLines[6].Separator = FALSE;
    strcpy(infoLines[7].Text, getMsg(MFilesDel));
    sprintf(infoLines[7].Data, "%d", diskInfo.noDelFiles);
    infoLines[7].Separator = FALSE;
    
    strcpy(infoLines[8].Text, getMsg(MDirInfo));
    infoLines[8].Separator = TRUE;
    
    strcpy(infoLines[9].Text, getMsg(MDirSys));
    if(!op.useDS)
      strcpy(infoLines[9].Data, getMsg(MDisabled));
    else
    {
      if(!dsOk)
        strcpy(infoLines[9].Data, getMsg(MAbsent));
      else
        strcpy(infoLines[9].Data, getMsg(MDS100));
    }
    infoLines[9].Separator = FALSE;
    
    strcpy(infoLines[10].Text, getMsg(MFreeAreaInfo));
    infoLines[10].Separator = TRUE;
    
    strcpy(infoLines[11].Text, getMsg(M1Trk));
    sprintf(infoLines[11].Data, "%d", diskInfo.firstFreeTrk);
    infoLines[11].Separator = FALSE;
    strcpy(infoLines[12].Text, getMsg(M1Sec));
    sprintf(infoLines[12].Data, "%d", diskInfo.firstFreeSec);
    infoLines[12].Separator = FALSE;
    strcpy(infoLines[13].Text, getMsg(MFreeSecs));
    sprintf(infoLines[13].Data, "%d", diskInfo.noFreeSecs);
    infoLines[13].Separator = FALSE;

    info->InfoLines = infoLines;
    info->InfoLinesNumber = sizeof(infoLines)/sizeof(infoLines[0]);
  }
}

int Manager::getFindData(PluginPanelItem **pPanelItem, int *pNoItems, int opMode)
{
  if(!readInfo()) return FALSE;
  // считаем число файлов и каталогов в текущем каталоге
  int noItems;
  int noFiles   = 0;
  int noFolders = 0;
  if(!op.useDS || !dsOk)
  {
    noItems = noFiles = this->noFiles;
  }
  else
  {
    for(int i = 0; i < this->noFolders; ++i)
      if(folderMap[i] == curFolderNum) ++noFolders;
    for(int i = 0; i < this->noFiles; ++i)
      if(fileMap[i] == curFolderNum) ++noFiles;
    noItems = noFolders + noFiles;
  }
  
  PluginPanelItem *item = new PluginPanelItem[noItems+1];
  memset(item, 0, (noItems + 1) * sizeof(PluginPanelItem));
  *pNoItems   = noItems+1;
  *pPanelItem = item;

  strcpy(item[0].FindData.cFileName, "..");
  item[0].FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

  item[0].CustomColumnNumber  = 1;
  item[0].CustomColumnData    = new LPSTR[item[0].CustomColumnNumber];
  item[0].CustomColumnData[0] = new char[3];
  strcpy(item[0].CustomColumnData[0], "..");

  if(noItems == 0) return TRUE;

  // считаем ширину столбца для отображения trdos'ного имени
  int nameColumnWidth = 12;
  if((opMode & OPM_FIND) == 0)
  {
    // откуда растут ноги не понятно, но при поиске файлов
    // внутренности if'а иногда роняют плагин
    PanelInfo panel;
    startupInfo.Control(this, FCTL_GETPANELINFO, &panel);
    if(panel.ViewMode == 4)
      // 24 - ширина остальных колонок,  2 - рамка панели
      nameColumnWidth = panel.PanelRect.right - panel.PanelRect.left - 2 - 24;
    if(nameColumnWidth < 12) nameColumnWidth = 12;
  }
  
  char* name = new char[nameColumnWidth+1];
  if(op.useDS && dsOk)
  {
    int folderNum = 0;
    for(int i = 0; i < noFolders; ++i)
    {
      int j = folderNum;
      for(; j < sizeof(folderMap); ++j) if(folderMap[j] == curFolderNum) break;

      folderNum = j;
      item[i+1].FindData.dwFileAttributes = folders[folderNum][0] != 0x01 ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN;
      strcpy(item[i+1].FindData.cFileName, pcFolders[folderNum]);

      item[i+1].CustomColumnNumber  = 1;
      item[i+1].CustomColumnData    = new LPSTR[item[i+1].CustomColumnNumber];
      item[i+1].CustomColumnData[0] = new char[nameColumnWidth+1];
      memset(name, ' ', nameColumnWidth+1);
      memcpy(name, folders[folderNum], 8);
      memcpy(name+nameColumnWidth-3, folders[folderNum]+8, 3);
      name[nameColumnWidth] = 0;
      strcpy(item[i+1].CustomColumnData[0], name);
      ++folderNum;
    }
  }
  int fNum = 0;
  int iNum;
  for(int k = 0; k < noFiles; ++k)
  {
    if(!op.useDS || !dsOk)
    {
      iNum = k + 1;
      fNum = k;
    }
    else
    {
      iNum = noFolders + k + 1;
      int j = fNum;
      for(; j < sizeof(fileMap); ++j) if(fileMap[j] == curFolderNum) break;
      fNum = j;
    }
    
    item[iNum].FindData.dwFileAttributes = files[fNum].name[0] == 0x01 ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
    strcpy(item[iNum].FindData.cFileName, pcFiles[fNum].name);
    item[iNum].FindData.nFileSize = files[fNum].size;

    item[iNum].CustomColumnNumber = 7;
    item[iNum].CustomColumnData = new LPSTR[item[iNum].CustomColumnNumber];
    
    item[iNum].CustomColumnData[0] = new char[nameColumnWidth+1];
    
    memset(name, ' ', nameColumnWidth+1);
    
    makeTrDosName(name, files[fNum], nameColumnWidth);
    strcpy(item[iNum].CustomColumnData[0], name);
    
    item[iNum].CustomColumnData[1] = new char[6];
    sprintf(item[iNum].CustomColumnData[1], "%5d", files[fNum].start);

    item[iNum].CustomColumnData[2] = new char[4];
    sprintf(item[iNum].CustomColumnData[2],
             "%3d",
             files[fNum].noSecs);

    item[iNum].CustomColumnData[3] = new char[4];
    sprintf(item[iNum].CustomColumnData[3], "%3d", files[fNum].trk);

    item[iNum].CustomColumnData[4] = new char[4];
    sprintf(item[iNum].CustomColumnData[4], "%3d", files[fNum].sec);

    
    char* description = detector->description(pcFiles[fNum].type);
    if(description)
    {
      item[iNum].CustomColumnData[5] = new char[strlen(description)+1];
      strcpy(item[iNum].CustomColumnData[5], description);
    }
    else
    {
      item[iNum].CustomColumnData[5] = new char[1];
      item[iNum].CustomColumnData[5][0] = 0;
    }
    char* comment = pcFiles[fNum].comment;
    if(comment)
    {
      item[iNum].CustomColumnData[6] = new char[strlen(comment)+1];
      strcpy(item[iNum].CustomColumnData[6], comment);
    }
    else
    {
      item[iNum].CustomColumnData[6] = new char[1];
      item[iNum].CustomColumnData[6][0] = 0;
    }
    ++fNum;
  }
  delete[] name;
  return TRUE;
}

void Manager::freeFindData(PluginPanelItem *panelItem, int  noItems)
{

  for (int i = 0; i < noItems; i++)
  {
    for(int j = 0; j < panelItem[i].CustomColumnNumber; ++j)
      delete[] panelItem[i].CustomColumnData[j];
    delete[] panelItem[i].CustomColumnData;
  }
  delete[] panelItem;
}

int Manager::processHostFile(PluginPanelItem* panelItem, int noItems, int opMode)
{
  return diskMenu();
}
