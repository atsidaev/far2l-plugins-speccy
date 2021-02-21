#ifndef creator_hpp
#define creator_hpp
#include <windows.h>

bool create(char *fileName, char *title, int writeProtect, int isDS, int format, int interleave, char *bootName, char *comment);

#endif