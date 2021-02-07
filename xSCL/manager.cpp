#include <windows.h>
#include "plugin.hpp"

#include "manager.hpp"
#include "detector.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "lang.hpp"

extern PluginStartupInfo startupInfo;
extern Detector*         detector;
extern Options           op;

Manager::Manager(char* fileName)
{
  lstrcpy(hostFileName, fileName);
  
  noFiles = 0;
  
  ZeroMemory(&lastModifed, sizeof(WIN32_FIND_DATA));
}

Manager::~Manager()
{
}

void Manager::getOpenPluginInfo(OpenPluginInfo* info)
{
  info->StructSize    = sizeof(*info);
  info->Flags         = OPIF_USEFILTER | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_SHOWPRESERVECASE;
  info->HostFile      = hostFileName;
  info->CurDir        = "";
  info->Format        = "SCL";
  info->StartSortMode = SM_UNSORTED;
  
  static char panelTitle[260];
  panelTitle[0] = 0;
  lstrcat(panelTitle, " SCL:");
  lstrcat(panelTitle, pointToName(hostFileName));
  lstrcat(panelTitle, " ");
  info->PanelTitle       = panelTitle;

  static PanelMode mode[10];
  static char*     columnTitles3[4];
  columnTitles3[0] = getMsg(MName);
  columnTitles3[1] = getMsg(MSectorSize);
  columnTitles3[2] = getMsg(MName);
  columnTitles3[3] = getMsg(MSectorSize);

  static char*     columnTitles4[4];
  columnTitles4[0] = getMsg(MName);
  columnTitles4[1] = getMsg(MSize);
  columnTitles4[2] = getMsg(MStart);
  columnTitles4[3] = getMsg(MSectorSize);

  static char*     columnTitles5[2];
  columnTitles5[0] = getMsg(MName);
  columnTitles5[1] = getMsg(MFormat);

  static char*     columnTitles6[2];
  columnTitles6[0] = getMsg(MName);
  columnTitles6[1] = getMsg(MComment);
  
  // C0 - trdos name
  // C1 - trdos 'start'
  // C2 - размер в секторах
  // C3 - описание типа
  // C4 - комментарий к файлу
  
  mode[3].ColumnTypes="N,C2,N,C2";
  mode[3].ColumnWidths="0,3,0,3";
  mode[3].ColumnTitles=columnTitles3;
  mode[3].StatusColumnTypes="C0,S,C2";
  mode[3].StatusColumnWidths="0,5,3";
  mode[3].AlignExtensions=TRUE;
  mode[3].FullScreen=FALSE;
  
  mode[4].ColumnTypes="C0,S,C1,C2";
  mode[4].ColumnWidths="0,5,5,3";
  mode[4].StatusColumnTypes="N,S,C2";
  mode[4].StatusColumnWidths="0,5,3";
  mode[4].ColumnTitles=columnTitles4;
  mode[4].FullScreen=FALSE;
  
  mode[5].ColumnTypes="C0,C3";
  mode[5].ColumnWidths="12,0";
  mode[5].StatusColumnTypes="N,S,C2";
  mode[5].StatusColumnWidths="0,5,3";
  mode[5].ColumnTitles=columnTitles5;
  mode[5].FullScreen=FALSE;

  mode[6].ColumnTypes="C0,C4";
  mode[6].ColumnWidths="12,0";
  mode[6].StatusColumnTypes="N,S,C2";
  mode[6].StatusColumnWidths="0,5,3";
  mode[6].ColumnTitles=columnTitles6;
  mode[6].FullScreen=FALSE;
  
  info->PanelModesArray = mode;
  info->PanelModesNumber = sizeof(mode)/sizeof(mode[0]);
  info->StartPanelMode = '0' + op.defaultPanelMode;
}

int Manager::getFindData(PluginPanelItem **pPanelItem, int *pNoItems, int opMode)
{
  if(!readInfo()) return FALSE;
  
  PluginPanelItem *item = new PluginPanelItem[noFiles];
  *pNoItems   = noFiles;
  *pPanelItem = item;
  
  if(noFiles == 0) return TRUE;
  
  // считаем ширину столбца для отображения trdos'ного имени
  PanelInfo panel;
  startupInfo.Control(this, FCTL_GETPANELINFO, &panel);
  int nameColumnWidth = 12;
  
  if(panel.ViewMode == 4)
  {
    nameColumnWidth = panel.PanelRect.right - panel.PanelRect.left - 2 - 15;
  }
  if(nameColumnWidth < 12) nameColumnWidth = 12;
  
  char* name = new char[nameColumnWidth+1];
  
  for(int fNum = 0; fNum < noFiles; ++fNum)
  {
    item[fNum].FindData.dwFileAttributes = files[fNum].name[0] == 0x01 ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
    lstrcpy(item[fNum].FindData.cFileName, pcFiles[fNum].name);
    item[fNum].FindData.nFileSizeLow = files[fNum].size;
    
    item[fNum].CustomColumnNumber = 5;
    item[fNum].CustomColumnData = new LPSTR[item[fNum].CustomColumnNumber];
    
    item[fNum].CustomColumnData[0] = new char[nameColumnWidth+1];
    
    FillMemory(name, nameColumnWidth+1, ' ');
    
    makeTrDosName(name, files[fNum], nameColumnWidth);
    lstrcpy(item[fNum].CustomColumnData[0], name);
    
    item[fNum].CustomColumnData[1] = new char[6];
    wsprintf(item[fNum].CustomColumnData[1], "%5d", files[fNum].start);
    
    item[fNum].CustomColumnData[2] = new char[4];
    wsprintf(item[fNum].CustomColumnData[2],
             "%3d",
             files[fNum].noSecs);
    
    char* description = detector->description(pcFiles[fNum].type);
    if(description)
    {
      item[fNum].CustomColumnData[3] = new char[lstrlen(description)+1];
      lstrcpy(item[fNum].CustomColumnData[3], description);
    }
    else
    {
      item[fNum].CustomColumnData[3] = new char[1];
      item[fNum].CustomColumnData[3][0] = 0;
    }

    char* comment = pcFiles[fNum].comment;
    if(comment)
    {
      item[fNum].CustomColumnData[4] = new char[lstrlen(comment)+1];
      lstrcpy(item[fNum].CustomColumnData[4], comment);
    }
    else
    {
      item[fNum].CustomColumnData[4] = new char[1];
      item[fNum].CustomColumnData[4][0] = 0;
    }
  }
  delete[] name;
  return TRUE;
}

void Manager::freeFindData(PluginPanelItem *panelItem, int noItems)
{
  for (int i = 0; i < noItems; i++)
  {
    delete[] panelItem[i].CustomColumnData[0];
    delete[] panelItem[i].CustomColumnData[1];
    delete[] panelItem[i].CustomColumnData[2];
    delete[] panelItem[i].CustomColumnData[3];
    delete[] panelItem[i].CustomColumnData[4];
    delete[] panelItem[i].CustomColumnData;
  }
  delete[] panelItem;
}
