/*
Eugene Kotlyarov <ek@oris.ru>
*/

#include <windows.h>

#define heapNEW	GetProcessHeap()
void *malloc(size_t size)
{
  return HeapAlloc(heapNEW, HEAP_ZERO_MEMORY, size);
}
void *realloc(void *block, size_t size)
{
  if (block)
    return HeapReAlloc(heapNEW,HEAP_ZERO_MEMORY,block,size);
  else
    return HeapAlloc(heapNEW,HEAP_ZERO_MEMORY, size);
}
void free(void *block)
{
  HeapFree(heapNEW,0,block);
}

#ifdef __cplusplus
void * operator new(size_t size)
{
  return HeapAlloc(heapNEW, HEAP_ZERO_MEMORY, size);
}

void operator delete(void *block)
{
  HeapFree(heapNEW,0,block);
}
#endif

void _pure_error_ () {};
