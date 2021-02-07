#include <windows.h>
#include "pluginold.hpp"
using namespace oldfar;

#include "FmtReader.hpp"
#include "tools.hpp"

#include <dlfcn.h>
#include "widestring.hpp"
#include "Formats/TRD/trdfmt.hpp"
#include "Formats/FDD/fddfmt.hpp"
#include "Formats/FDI/fdifmt.hpp"
#include "Formats/TELEDISK/tdfmt.hpp"

extern PluginStartupInfo startupInfo;

FmtReader::FmtReader()
{
  plugins[noPlugins].handle  = (HMODULE)noPlugins;
  plugins[noPlugins].isImage = trd_isImage;
  plugins[noPlugins].init    = trd_init;
  plugins[noPlugins].cleanup = trd_cleanup;
  plugins[noPlugins].reload  = trd_reload;
  plugins[noPlugins].open    = trd_open;
  plugins[noPlugins].close   = trd_close;
  plugins[noPlugins].read    = trd_read;
  plugins[noPlugins].write   = trd_write;

  plugins[noPlugins].getFormatName = trd_getFormatName;
  plugins[noPlugins].isProtected   = trd_isProtected;
  plugins[noPlugins].protect       = trd_protect;
  noPlugins++;
  
  plugins[noPlugins].handle  = (HMODULE)noPlugins;
  plugins[noPlugins].isImage = fdd_isImage;
  plugins[noPlugins].init    = fdd_init;
  plugins[noPlugins].cleanup = fdd_cleanup;
  plugins[noPlugins].reload  = fdd_reload;
  plugins[noPlugins].open    = fdd_open;
  plugins[noPlugins].close   = fdd_close;
  plugins[noPlugins].read    = fdd_read;
  plugins[noPlugins].write   = fdd_write;

  plugins[noPlugins].getFormatName = fdd_getFormatName;
  plugins[noPlugins].isProtected   = fdd_isProtected;
  plugins[noPlugins].protect       = fdd_protect;
  noPlugins++;
  
  plugins[noPlugins].handle  = (HMODULE)noPlugins;
  plugins[noPlugins].isImage = fdi_isImage;
  plugins[noPlugins].init    = fdi_init;
  plugins[noPlugins].cleanup = fdi_cleanup;
  plugins[noPlugins].reload  = fdi_reload;
  plugins[noPlugins].open    = fdi_open;
  plugins[noPlugins].close   = fdi_close;
  plugins[noPlugins].read    = fdi_read;
  plugins[noPlugins].write   = fdi_write;

  plugins[noPlugins].getFormatName = fdi_getFormatName;
  plugins[noPlugins].isProtected   = fdi_isProtected;
  plugins[noPlugins].protect       = fdi_protect;
  noPlugins++;
  
  plugins[noPlugins].handle  = (HMODULE)noPlugins;
  plugins[noPlugins].isImage = td_isImage;
  plugins[noPlugins].init    = td_init;
  plugins[noPlugins].cleanup = td_cleanup;
  plugins[noPlugins].reload  = td_reload;
  plugins[noPlugins].open    = td_open;
  plugins[noPlugins].close   = td_close;
  plugins[noPlugins].read    = td_read;
  plugins[noPlugins].write   = td_write;

  plugins[noPlugins].getFormatName = td_getFormatName;
  plugins[noPlugins].isProtected   = td_isProtected;
  plugins[noPlugins].protect       = td_protect;
  noPlugins++;
}

/* Do not want to debug all this. Formats are simply hardcoded above */
/*FmtReader::FmtReader(char *path)
{
  noPlugins = 0;
  char pluginPath[300];
  char pluginMask[300];
  
  strcpy(pluginPath, path);
  strcpy(pointToName(pluginPath), "Formats/");
  
  strcpy(pluginMask, pluginPath);
  strcat(pluginMask, "*.fmt");

  WIN32_FIND_DATA data;
  HANDLE foundFile = FindFirstFile(_W(pluginMask).c_str(), &data);
  trdlog << "Found file: " << foundFile << std::endl;  
  int done = (foundFile == INVALID_HANDLE_VALUE);
  while(!done)
  {
    char pluginName[300];
    strcpy(pluginName, pluginPath);
    strcat(pluginName, _N(data.cFileName).c_str());
    
    trdlog << "Format found: " << pluginName << std::endl;
    auto hPlugin = dlopen(pluginName, RTLD_NOW);
    if(hPlugin != NULL)
    {
      trdlog << "Format valid" << std::endl;
      plugins[noPlugins].handle  = hPlugin;
      plugins[noPlugins].isImage = (IS_IMAGE)dlsym(hPlugin,"fmt_isImage");
      plugins[noPlugins].init    = (INIT)    dlsym(hPlugin,"fmt_init");
      plugins[noPlugins].cleanup = (CLEANUP) dlsym(hPlugin,"fmt_cleanup");
      plugins[noPlugins].reload  = (RELOAD)  dlsym(hPlugin,"fmt_reload");
      plugins[noPlugins].open    = (OPEN)    dlsym(hPlugin,"fmt_open");
      plugins[noPlugins].close   = (CLOSE)   dlsym(hPlugin,"fmt_close");
      plugins[noPlugins].read    = (READ)    dlsym(hPlugin,"fmt_read");
      plugins[noPlugins].write   = (WRITE)   dlsym(hPlugin,"fmt_write");

      plugins[noPlugins].getFormatName = (GET_FORMAT_NAME)dlsym(hPlugin,"fmt_getFormatName");
      plugins[noPlugins].isProtected   = (IS_PROTECTED)   dlsym(hPlugin,"fmt_isProtected");
      plugins[noPlugins].protect       = (PROTECT)        dlsym(hPlugin,"fmt_protect");
      
      dlclose(hPlugin);
      
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
      trdlog << "Recognized plugins: " << noPlugins << std::endl;
      if(noPlugins == 16) break;
    }
    else
      trdlog << "Error " << dlerror() << std::endl;
    done = !FindNextFile(foundFile, &data);
  }
  FindClose(foundFile);
}*/

FmtReader::~FmtReader()
{
  // for(int i = 0; i < noPlugins; ++i) FreeLibrary(plugins[i].handle);
}

FmtPlugin* FmtReader::isImage(char *fileName, const BYTE *data, int size)
{
  for(int i = 0; i < noPlugins; ++i)
    if(plugins[i].isImage(fileName, data, size)) return &plugins[i];
  return NULL;
}
