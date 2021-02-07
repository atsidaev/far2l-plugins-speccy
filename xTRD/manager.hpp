#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <windows.h>
#include "plugin.hpp"
#include "fmtReader.hpp"
#include "types.hpp"

class Manager
{
  public:
    Manager(char* fileName, FmtPlugin *fmtPlugin);
    ~Manager();
    void getOpenPluginInfo(OpenPluginInfo *info);
    
    int  getFindData      (PluginPanelItem **pPanelItem, int *pNoItems, int opMode);
    void freeFindData     (PluginPanelItem *panelItem,   int  noItems);
    int  setDirectory     (char* dirName, int opMode);
    int  makeDirectory    (char *dirName, int opMode);
    int  deleteFiles      (PluginPanelItem *panelItem, int noItems, int opMode);
    int  getFiles         (PluginPanelItem *panelItem, int noItems, int move, char *destPath, int opMode);
    int  putFiles         (PluginPanelItem *panelItem, int noItems, int move, int opMode);
    int  processKey       (int key, unsigned int controlState);
    int  diskMenu         (void);
    int  processHostFile  (PluginPanelItem* panelItem, int noItems, int opMode);
  private:
    char            hostFileName[260];
    char            curFolder   [260];
    int             curFolderNum;
    WIN32_FIND_DATA lastModifed;
    FmtPlugin       *fmt;
    int             noFiles, noDelFiles;
    HANDLE          img;

    FileHdr         files  [142];
    ExtFileHdr      pcFiles[142];
    DiskHdr         diskInfo;

    BYTE            fileMap  [142];
    BYTE            folderMap[127];
    char            folders  [127][11];
    char            pcFolders[127][15];
    int             noFolders, noDelFolders;

    BYTE            zeroTrk[16*sectorSize];
    
    WORD            dsCRC;
    bool            dsOk;

    void getBytes     (void* to, int size);
    bool readInfo     (void);
    void move         (void);
    void writeInfo    (void);
    bool openHostFile (void);
    void closeHostFile(void);
    void makePCNames  (void);
    WORD calcDScrc    (void);
    bool checkEntry   (char num, bool del, bool *checked);
    bool checkDS      (void);
    bool checkDisk    (void);
    
    ExitCode markFolder(int fNum);

    ExitCode getFolder (int fNum, bool isMove, char* to);
    ExitCode getFile   (int fNum, bool isMove, char* to);

    bool isDiskFull   (int noAddFiles, int noSecs);
    bool makeFolder   (char* folderName, BYTE folderNum);
   
    ExitCode writeFile(const SCLHdr& h, HANDLE file, BYTE folderNum);
    ExitCode putFile  (char* fileName,   BYTE folderNum, bool move);
    ExitCode putFolder(char* folderName, BYTE folderNum, bool move);

    bool read         (int trk, int sec, BYTE* buf);
    bool write        (int trk, int sec, BYTE* buf);
};

#endif
