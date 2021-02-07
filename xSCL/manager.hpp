#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <windows.h>
#include "plugin.hpp"
#include "types.hpp"

class Manager
{
  public:
    Manager(char* fileName);
    ~Manager();
    void getOpenPluginInfo(OpenPluginInfo *info);
    int  getFindData      (PluginPanelItem **pPanelItem, int *pNoItems, int opMode);
    void freeFindData     (PluginPanelItem *panelItem,   int  noItems);
    
    int  deleteFiles      (PluginPanelItem *panelItem, int noItems, int opMode);
    int  getFiles         (PluginPanelItem *panelItem, int noItems, int move, char *destPath, int opMode);
    int  putFiles         (PluginPanelItem *panelItem, int noItems, int move, int opMode);
    int  processKey       (int key, unsigned int controlState);
    
  private:
    bool readInfo         (void);
    void makePCNames      (void);
    bool openHostFile     (void);
    void closeHostFile    (void);
    
    bool deleteFilesImpl  (BYTE noItems);
    
    // action:                        return:
    //          0 - спросить                     1 - ok/overwrite
    //          1 - overwriteAll                 0 - skip
    //          2 - skipAll                     -1 - cancel
    int createFile        (HANDLE& file, char* name, int& action);
    // hdrs - массив заголовков существующих файлов
    // hdr  - заголовок копируемого файла
    
    // action:                        return:
    //          0 - спросить                     1 - write
    //          1 - writeAll                     0 - skip
    //          2 - skipAll                     -1 - cancel
    
    int checkExistingFiles(FileHdr* hdrs, FileHdr hdr, int no_files, int& allActions);

    // возвращают контрольную сумму
    DWORD copyFile         (int fileNum, HANDLE file, bool skipGarbage = false);
    DWORD writeSCLHeader   (HANDLE file, BYTE no_files);
    
    char            hostFileName[300];
    HANDLE          hostFile;
    WIN32_FIND_DATA lastModifed;
    bool            isReadOnly;
    
    BYTE       noFiles;
    FileHdr    files[255];
    ExtFileHdr pcFiles[255];
};

#endif
