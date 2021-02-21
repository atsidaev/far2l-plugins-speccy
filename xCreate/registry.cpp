#include <windows.h>
#include "registry.hpp"

extern char rootKeyName[80];

HKEY createKey(HKEY root)
{
  HKEY key;
  DWORD  actionPerformed;
  RegCreateKeyEx(root,
                 rootKeyName,
                 0,
                 NULL,
                 0,
                 KEY_WRITE,
                 NULL,
                 &key,
                 &actionPerformed);
 return key;
}

HKEY openKey(HKEY root)
{
  HKEY key;
  if(RegOpenKeyEx(root, rootKeyName, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
    return (NULL);
  else
    return key;
}

void setNumber(HKEY root, char* valueName, DWORD value)
{
  HKEY key = createKey(root);
  RegSetValueEx(key,
                valueName,
                0,
                REG_DWORD,
                (BYTE*)&value,
                sizeof(value));
  RegCloseKey(key);
}

DWORD getNumber(HKEY root, char* valueName, DWORD defValue)
{
  HKEY key = openKey(root);
  
  DWORD type, value, size = sizeof(defValue);
  int exitCode = RegQueryValueEx(key, valueName, 0, &type, (BYTE*)&value, &size);
  RegCloseKey(key);
  
  if(key == NULL || exitCode != ERROR_SUCCESS || type != REG_DWORD)
    return defValue;
  else
    return value;
}

void setString(HKEY root, char* valueName, char* value)
{
  HKEY key = createKey(root);
  RegSetValueEx(key, valueName, 0, REG_SZ, value, lstrlen(value)+1);
  RegCloseKey(key);
}

bool getString(HKEY root, char* valueName, char* value, char* defValue, DWORD size)
{
  HKEY key = openKey(root);
  DWORD type;
  int exitCode = RegQueryValueEx(key, valueName, 0, &type, value, &size);
  RegCloseKey(key);

  if(key == NULL || exitCode != ERROR_SUCCESS || type != REG_SZ)
  {
    lstrcpy(value, defValue);
    return false;
  }
  else
    return true;
}
