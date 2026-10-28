#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;
#include "types.hpp"
#include "../shared/volumes.hpp"

class Manager
{
  public:
    Manager(char* fileName, int offset);
    ~Manager();
    void getOpenPluginInfo(OpenPluginInfo *info);
    
    int  getFindData      (PluginPanelItem **pPanelItem, int *pNoItems, int opMode);
    void freeFindData     (PluginPanelItem *panelItem,   int  noItems);
    int getFiles(PluginPanelItem * panelItem, int noItems, int move, char * destPath, int opMode);

    bool isValid() {
      return readInfo();
    }

  private:
    std::string hostFileName, panelTitle;
    speccy::Bytes archive;
    int noFiles = 0;
    bool loaded = false, parsed = false;
    FileHdr files[255]{};
    ExtFileHdr pcFiles[255]{};
    PackInfo infos[255]{};

    bool readInfo     (void);
    void makePCNames  (void);

    bool extractFile  (int fNum, BYTE* buffer, int size);

};

#endif
