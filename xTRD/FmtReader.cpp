#include <windows.h>
#include "plugin.hpp"
#include "FmtReader.hpp"
#include "tools.hpp"

extern PluginStartupInfo startupInfo;

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

    HMODULE hPlugin = LoadLibrary(pluginName);
    if(hPlugin != NULL)
    {
      
      plugins[noPlugins].handle  = hPlugin;
      plugins[noPlugins].isImage = (IS_IMAGE)GetProcAddress(hPlugin,"isImage");
      plugins[noPlugins].init    = (INIT)    GetProcAddress(hPlugin,"init");
      plugins[noPlugins].cleanup = (CLEANUP) GetProcAddress(hPlugin,"cleanup");
      plugins[noPlugins].reload  = (RELOAD)  GetProcAddress(hPlugin,"reload");
      plugins[noPlugins].open    = (OPEN)    GetProcAddress(hPlugin,"open");
      plugins[noPlugins].close   = (CLOSE)   GetProcAddress(hPlugin,"close");
      plugins[noPlugins].read    = (READ)    GetProcAddress(hPlugin,"read");
      plugins[noPlugins].write   = (WRITE)   GetProcAddress(hPlugin,"write");

      plugins[noPlugins].getFormatName = (GET_FORMAT_NAME)GetProcAddress(hPlugin,"getFormatName");
      plugins[noPlugins].isProtected   = (IS_PROTECTED)   GetProcAddress(hPlugin,"isProtected");
      plugins[noPlugins].protect       = (PROTECT)        GetProcAddress(hPlugin,"protect");
      
      if(plugins[noPlugins].isImage != NULL &&
         plugins[noPlugins].init    != NULL &&
         plugins[noPlugins].cleanup != NULL &&
         plugins[noPlugins].reload  != NULL &&
         plugins[noPlugins].open    != NULL &&
         plugins[noPlugins].close   != NULL &&
         plugins[noPlugins].read    != NULL &&
         plugins[noPlugins].write   != NULL &&
         plugins[noPlugins].getFormatName != NULL &&
         plugins[noPlugins].isProtected   != NULL &&
         plugins[noPlugins].protect       != NULL) ++noPlugins;
      if(noPlugins == 16) break;
    }
    done = !FindNextFile(foundFile, &data);
  }
  FindClose(foundFile);
}

FmtReader::~FmtReader()
{
  for(int i = 0; i < noPlugins; ++i) FreeLibrary(plugins[i].handle);
}

FmtPlugin* FmtReader::isImage(char *fileName, const BYTE *data, int size)
{
  for(int i = 0; i < noPlugins; ++i)
    if(plugins[i].isImage(fileName, data, size)) return &plugins[i];
  return NULL;
}
