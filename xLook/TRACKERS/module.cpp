#include <windows.h>

#include "module.hpp"

void trimRight(char* str)
{
  int len = strlen(str);

  while(len && str[len - 1] == ' ')
    --len;
  str[len] = 0;
}

int lstrcmpn(char* s1, char* s2, int max)
{
  for(int i = 0; i < max; ++i)
  {
    if(s1[i] != s2[i]) return s1[i]-s2[i];
    if(s1[i] == 0) return 0;
  }
  return 0;
}

bool search_with_table(const BYTE * data_ptr, const BYTE * table_ptr, int size) {
  while(*table_ptr != 0xff) {
    if(*table_ptr == 0xfe) {
      ++table_ptr;
      int skip = *table_ptr++;
      if(skip > size)
        return false;
      data_ptr += skip;
      size -= skip;
      continue;
    }

    if(*table_ptr == 0xfd) {
      ++table_ptr;
      if(size < 2)
        return false;
      data_ptr += 2;
      size -= 2;
      continue;
    }
    if(size-- <= 0 || *data_ptr++ != *table_ptr++)
      return false;
  }
  return true;
}
