#include <windows.h>
#include "fmtReader.hpp"
#include "tools.hpp"

FmtReader::FmtReader(char *path)
{
  noPlugins = 0;
  char pluginPath[300];
  char pluginMask[300];
  
  lstrcpy(pluginPath, path);
  lstrcpy(pointToName(pluginPath), "Formats\\");
  
  lstrcpy(pluginMask, pluginPath);
  lstrcat(pluginMask, "*.fmt");

  WIN32_FIND_DATA data;
  HANDLE foundFile = FindFirstFile(pluginMask, &data);
  
  int done = (foundFile == INVALID_HANDLE_VALUE);
  while(!done)
  {
    char pluginName[300];
    lstrcpy(pluginName, pluginPath);
    lstrcat(pluginName, data.cFileName);

    HINSTANCE hPlugin = LoadLibrary(pluginName);
    if(hPlugin != NULL)
    {
      plugins[noPlugins].handle         = hPlugin;
      plugins[noPlugins].isImage        = (IS_IMAGE)         GetProcAddress(hPlugin,"isImage");
      plugins[noPlugins].openSubPlugin  = (OPEN_SUB_PLUGIN)  GetProcAddress(hPlugin,"openSubPlugin");
      plugins[noPlugins].closeSubPlugin = (CLOSE_SUB_PLUGIN) GetProcAddress(hPlugin,"closeSubPlugin");
      plugins[noPlugins].reload         = (RELOAD)           GetProcAddress(hPlugin,"reload");
      plugins[noPlugins].openFile       = (OPEN_FILE)        GetProcAddress(hPlugin,"openFile");
      plugins[noPlugins].closeFile      = (CLOSE_FILE)       GetProcAddress(hPlugin,"closeFile");
      plugins[noPlugins].read           = (READ)             GetProcAddress(hPlugin,"read");
      plugins[noPlugins].write          = (WRITE)            GetProcAddress(hPlugin,"write");
      plugins[noPlugins].getFormatName  = (GET_FORMAT_NAME)  GetProcAddress(hPlugin,"getFormatName");

      if(plugins[noPlugins].isImage        != NULL &&
         plugins[noPlugins].openSubPlugin  != NULL &&
         plugins[noPlugins].closeSubPlugin != NULL &&
         plugins[noPlugins].reload         != NULL &&
         plugins[noPlugins].openFile       != NULL &&
         plugins[noPlugins].closeFile      != NULL &&
         plugins[noPlugins].read           != NULL &&
         plugins[noPlugins].write          != NULL &&
         plugins[noPlugins].getFormatName  != NULL) ++noPlugins;
      
      if(noPlugins == 16) break;
    }
    done = !FindNextFile(foundFile, &data);
  }
  FindClose(foundFile);
}

FmtPlugin* FmtReader::isImage(char *fileName, const unsigned char *data, int size)
{
  for(int i = 0; i < noPlugins; ++i)
    if(plugins[i].isImage(fileName, data, size)) return &plugins[i];
  return NULL;
}
