#ifndef FmtReader_hpp
#define FmtReader_hpp

#include "fmtPlugin.hpp"

class FmtReader
{
 public:
  FmtReader(char *path);
  FmtPlugin* isImage(char *fileName, const unsigned char *data, int size);
 private:
  int       noPlugins;
  FmtPlugin plugins[16];
};

#endif
