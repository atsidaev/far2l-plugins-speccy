#ifndef manager_hpp
#define manager_hpp

#include "plugin.hpp"
#include "FmtPlugin.hpp"
#include "iSDOS.hpp"

class Manager
{
  public:
    Manager(char* fileName, FmtPlugin* fmtPlugin);
    ~Manager();

    void getOpenPluginInfo(OpenPluginInfo  *info);
    int  getFindData      (PluginPanelItem **pPanelItem, int *pNoItems, int opMode);
    void freeFindData     (PluginPanelItem *panelItem,   int  noItems);
    int  setDirectory     (char* dirName, int opMode);

    bool openHostFile     (void);
    bool closeHostFile    (void);
    bool readBlock        (u16 num, u8* buf);
    bool writeBlock       (u16 num, u8* buf);
    
  private:
    FmtPlugin* filer;
    HANDLE     img;
    
    char       hostFileName[300];
    char       curDir[300];

    u16        noFreeBlocks;
    UniHdr     device_sys_hdr;
    u8*        device_sys;
    
    DiskHdr    dsk;
    UniHdr     files[128];
    
    WIN32_FIND_DATA lastModifed;

  private:
    bool readFolder      (const UniHdr& h, UniHdr* dest);
    bool readFolder      (u16 firstBlock,  UniHdr* dest);
    bool writeFolder     (UniHdr* pDir);
    bool readInfo        (void);
    u16  getNoFreeBlocks (void);
    void read_device_sys (void);
    void write_device_sys(void);
    
  public:
    int writeFile (const char *name, const UniHdr& hdr);
    int getOneFile(UniHdr& h, UniHdr* pDir, const char* to_path, const char* from_path, int move, Action& action, int opMode);
    int getFiles  (PluginPanelItem *panelItem, int noItems, int move, char *destPath, int opMode);

  private:
    bool   isFreeBlock      (u16 n);
    bool   markBlock        (u16 n);
    bool   unmarkBlock      (u16 n);
    int    findFreeBlocks   (u16 n);
    bool   findMaxFreeBlocks(u16 n, u16* firstFoundBlock, u16* noFoundBlocks);

  public:
    int deleteOneFile(UniHdr& h, UniHdr* dir, const char* dirName, int opMode, Action& actProtected, Action& actFolder);
    int deleteFiles(PluginPanelItem *panelItem, int noItems, int opMode);

  private:
    bool reserveSpaceForFiles(UniHdr* pDir, u16 noAddFiles);
    int  makeFolder          (UniHdr* pDir, const char* name);
  public:
    int  makeDirectory(char *dirName, int opMode);
    
  private:
    int putOneFile(UniHdr* pDir, WIN32_FIND_DATA& file, const char* fromDir, const char* dirName, int move, Action& action, int opMode);
    int putOneFolder(UniHdr* pDir, WIN32_FIND_DATA& file, const char* fromDir, const char* dirName, int move, Action& action, int opMode);
  public:
    int putFiles  (PluginPanelItem *panelItem, int noItems, int move, int opMode);
    int processKey(int key, unsigned int controlState);
    
};

#endif
