#ifndef registry_hpp
#define registry_hpp
#include <windows.h>

class Registry
{
  public:
    Registry(char* rootKeyName, wchar_t* pluginPath);
    void  setNumber(HKEY root, wchar_t* valueName, DWORD value);
    void  setString(HKEY root, wchar_t* valueName, char* value);
    
    DWORD getNumber(HKEY root, wchar_t* valueName, DWORD defValue);
    bool  getString(HKEY root, wchar_t* valueName, char* value, char* defValue, DWORD size);
  private:
    HKEY  createKey(HKEY root);
    HKEY  openKey  (HKEY root);

    wchar_t pluginRootKeyName[300];
};

#endif
