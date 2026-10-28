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

Manager::Manager(char * fileName) : hostFileName(fileName) {
  loaded = speccy::zipVolumes(fileName, archive);
  panelTitle = " xZXZIP:" + speccy::fileName(hostFileName) + " ";
}

Manager::~Manager() = default;

void Manager::getOpenPluginInfo(OpenPluginInfo* info)
{
  info->StructSize    = sizeof(*info);
  info->Flags         = OPIF_USEFILTER | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_SHOWNAMESONLY | OPIF_SHOWPRESERVECASE;
  info->HostFile = hostFileName.c_str();
  info->CurDir        = "";
  info->Format        = "ZXZIP";
  info->StartSortMode = SM_UNSORTED;

  info->PanelTitle = panelTitle.c_str();

  static PanelMode   mode[10];
  static char*       columnTitles3[4];
  columnTitles3[0] = getMsg(MName);
  columnTitles3[1] = getMsg(MSectorSize);
  columnTitles3[2] = getMsg(MName);
  columnTitles3[3] = getMsg(MSectorSize);

  static char*       columnTitles4[6];
  columnTitles4[0] = getMsg(MName);
  columnTitles4[1] = getMsg(MSize);
  columnTitles4[2] = getMsg(MStart);
  columnTitles4[3] = getMsg(MSectorSize);
  columnTitles4[4] = getMsg(MPackedSize);
  columnTitles4[5] = getMsg(MMethod);

  static char*       columnTitles5[2];
  columnTitles5[0] = getMsg(MName);
  columnTitles5[1] = getMsg(MFormat);

  static char*       columnTitles6[2];
  columnTitles6[0] = getMsg(MName);
  columnTitles6[1] = getMsg(MComment);

  // C0 - trdos name
  // C1 - trdos 'start'
  // C2 - размер в секторах
  // C3 - packedSize
  // C4 - метод упаковки
  // C5 - описание типа
  // C6 - комментарий к файлу

  mode[3].ColumnTypes="N,C2,N,C2";
  mode[3].ColumnWidths="0,3,0,3";
  mode[3].ColumnTitles=columnTitles3;
  mode[3].StatusColumnTypes="C0,S,C2";
  mode[3].StatusColumnWidths="0,5,3";
  mode[3].AlignExtensions=TRUE;
  mode[3].FullScreen=FALSE;

  mode[4].ColumnTypes="C0,S,C1,C2,C3,C4";
  mode[4].ColumnWidths="0,5,5,3,5,3";
  mode[4].StatusColumnTypes="N,S,C2";
  mode[4].StatusColumnWidths="0,5,3";
  mode[4].ColumnTitles=columnTitles4;
  mode[4].FullScreen=FALSE;

  mode[5].ColumnTypes="C0,C5";
  mode[5].ColumnWidths="12,0";
  mode[5].StatusColumnTypes="N,S,C2";
  mode[5].StatusColumnWidths="0,5,3";
  mode[5].ColumnTitles=columnTitles5;
  mode[5].FullScreen=FALSE;

  mode[6].ColumnTypes="C0,C6";
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
  memset(item, 0, sizeof(PluginPanelItem) * noFiles);
  *pNoItems   = noFiles;
  *pPanelItem = item;

  if(noFiles == 0) return TRUE;

  // считаем ширину столбца для отображения trdos'ного имени
  PanelInfo panel;
  startupInfo.Control(this, FCTL_GETPANELINFO, &panel);
  int nameColumnWidth = 12;

  if(panel.ViewMode == 4)
  {
    // 25 - ширина остальных колонок,  2 - рамка панели
    nameColumnWidth = panel.PanelRect.right - panel.PanelRect.left - 2 - 25;
  }
  if(nameColumnWidth < 12) nameColumnWidth = 12;

  char* name = new char[nameColumnWidth+1];

  for(int fNum = 0; fNum < noFiles; ++fNum)
  {
    strcpy(item[fNum].FindData.cFileName, pcFiles[fNum].name);
    item[fNum].FindData.nFileSize = files[fNum].size;

    item[fNum].CustomColumnNumber = 7;
    item[fNum].CustomColumnData = new LPSTR[item[fNum].CustomColumnNumber];

    item[fNum].CustomColumnData[0] = new char[nameColumnWidth+1];

    memset(name, ' ', nameColumnWidth+1);

    makeTrDosName(name, files[fNum], nameColumnWidth);
    strcpy(item[fNum].CustomColumnData[0], name);

    item[fNum].CustomColumnData[1] = new char[6];
    sprintf(item[fNum].CustomColumnData[1], "%5d", files[fNum].start);

    item[fNum].CustomColumnData[2] = new char[4];
    sprintf(item[fNum].CustomColumnData[2],
             "%3d",
             files[fNum].noSecs);

    item[fNum].CustomColumnData[3] = new char[6];
    sprintf(item[fNum].CustomColumnData[3], "%5d", files[fNum].packedSize);

    char* method = new char[4];
    switch(files[fNum].pMethod)
    {
      case 0:
                strcpy(method, "Str");
                break;
      case 1:
                strcpy(method, "LZP");
                break;
      case 2:
                strcpy(method, "Shr");
                break;
      case 3:
                if(files[fNum].flags & 0x01)
                 strcpy(method, "Imp");
                else
                 strcpy(method, "imp");
                break;
      default:
                strcpy(method, "???");
    }
    item[fNum].CustomColumnData[4] = method;

    const char * description = detector->description(pcFiles[fNum].type);
    if(description)
    {
      item[fNum].CustomColumnData[5] = new char[strlen(description)+1];
      strcpy(item[fNum].CustomColumnData[5], description);
    }
    else
    {
      item[fNum].CustomColumnData[5] = new char[1];
      item[fNum].CustomColumnData[5][0] = 0;
    }

    char* comment = pcFiles[fNum].comment;
    if(comment)
    {
      item[fNum].CustomColumnData[6] = new char[strlen(comment)+1];
      strcpy(item[fNum].CustomColumnData[6], comment);
    }
    else
    {
      item[fNum].CustomColumnData[6] = new char[1];
      item[fNum].CustomColumnData[6][0] = 0;
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
    delete[] panelItem[i].CustomColumnData[5];
    delete[] panelItem[i].CustomColumnData[6];
    delete[] panelItem[i].CustomColumnData;
  }
  delete[] panelItem;
}

bool Manager::readInfo() {
  if(parsed)
    return loaded;
  parsed = true;
  if(!loaded)
    return false;
  size_t offset = 0;
  while(offset < archive.size()) {

    if(std::all_of(archive.begin() + offset, archive.end(), [](BYTE b) {
         return b == 0;
       }))
      break;
    if(noFiles == 255 || archive.size() - offset < sizeof(ZipedFileHdr))
      return loaded = false;
    ZipedFileHdr header{};
    memcpy(&header, archive.data() + offset, sizeof(header));
    if(!isValidZippedFile(header))
      return loaded = false;
    offset += sizeof(header);
    if(header.packedSize > archive.size() - offset ||
       (header.pMethod == 0 && header.packedSize != fileSize(header)))
      return loaded = false;
    files[noFiles] = header;
    pcFiles[noFiles].offset = offset;
    ++noFiles;
    offset += header.packedSize;
  }
  makePCNames();
  return loaded;
}
