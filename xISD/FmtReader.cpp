#include <windows.h>
#include "fmtReader.hpp"
#include "tools.hpp"

#include "Formats/IMG/img.hpp"
#include "Formats/FDI/fdifmt.hpp"

FmtReader::FmtReader(char *path)
{
  plugins[noPlugins].handle         = (HMODULE)noPlugins;
  plugins[noPlugins].isImage        = img_isImage;
  plugins[noPlugins].openSubPlugin  = img_openSubPlugin;
  plugins[noPlugins].closeSubPlugin = img_closeSubPlugin;
  plugins[noPlugins].reload         = img_reload;
  plugins[noPlugins].openFile       = img_openFile;
  plugins[noPlugins].closeFile      = img_closeFile;
  plugins[noPlugins].read           = img_read;
  plugins[noPlugins].write          = img_write;
  plugins[noPlugins].getFormatName  = img_getFormatName;
  noPlugins++;

  plugins[noPlugins].handle         = (HMODULE)noPlugins;
  plugins[noPlugins].isImage        = fdi_isImage;
  plugins[noPlugins].openSubPlugin  = fdi_openSubPlugin;
  plugins[noPlugins].closeSubPlugin = fdi_closeSubPlugin;
  plugins[noPlugins].reload         = fdi_reload;
  plugins[noPlugins].openFile       = fdi_openFile;
  plugins[noPlugins].closeFile      = fdi_closeFile;
  plugins[noPlugins].read           = fdi_read;
  plugins[noPlugins].write          = fdi_write;
  plugins[noPlugins].getFormatName  = fdi_getFormatName;
  noPlugins++;
}

FmtPlugin* FmtReader::isImage(char *fileName, const unsigned char *data, int size)
{
  for(int i = 0; i < noPlugins; ++i)
    if(plugins[i].isImage(fileName, data, size)) return &plugins[i];
  return NULL;
}
