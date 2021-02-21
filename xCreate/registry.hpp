#ifndef registry_hpp
#define registry_hpp
#include <windows.h>

HKEY createKey(HKEY root);
HKEY openKey  (HKEY root);

void  setNumber(HKEY root, char* valueName, DWORD value);
void  setString(HKEY root, char* valueName, char* value);
    
DWORD getNumber(HKEY root, char* valueName, DWORD defValue);
bool  getString(HKEY root, char* valueName, char* value, char* defValue, DWORD size);

#endif