#include "iterator.hpp"
#include "iSDOS_tools.hpp"

Iterator::Iterator(FmtPlugin* filer, HANDLE img, const UniHdr h)
{
  u32 size;
  u16 firstBlock, descr1stBlock;
  
  if(isDir(h))
  {
    size = h.dir.size;
    firstBlock = h.dir.firstBlock;
    descr1stBlock = h.dir.descr1stBlock;
  }
  else
  {
    size = getSize(h);
    firstBlock = descr1stBlock = h.file.firstBlock;
  }
  
  if(h.attr & FLAG_SOLID)
  {
    fragmentsList   = 0;
    
    noFragments     = 1;
    noBlocks        = size/blockSize + ((size%blockSize) ? 1 : 0);
    startBlock      = firstBlock;
  }
  else
  {
    fragmentsList = (u8*)malloc(blockSize);
    filer->read(img, descr1stBlock, fragmentsList);
    
    noFragments = fragmentsList[0];
    startBlock  = *(u16*)(fragmentsList+1);
    noBlocks    = fragmentsList[3];
  }
  currentBlock    = 0;
  currentFragment = 0;
}

Iterator::Iterator()
{
  fragmentsList   = 0;
  
  noFragments     = 0;
  noBlocks        = 0;
  startBlock      = 0;
  
  currentBlock    = 0;
  currentFragment = 0;
}

Iterator::~Iterator()
{
  if(fragmentsList) free(fragmentsList);
}

const int Iterator::operator* () const
{
  return startBlock + currentBlock;
}

Iterator& Iterator::operator++ ()
{
  if(currentFragment == noFragments && currentBlock == noBlocks) return *this;
  
  ++currentBlock;
  if(currentBlock == noBlocks)
  {
    ++currentFragment;
    if(currentFragment < noFragments)
    {
      currentBlock = 0;
      startBlock   = *(u16*)(fragmentsList+3*currentFragment+1);
      noBlocks     = fragmentsList[3*currentFragment+3];
    }
  }
  return *this;
}

bool Iterator::operator== (const Iterator&) const
{
  return (currentFragment == noFragments && currentBlock == noBlocks);
}

bool Iterator::operator!= (const Iterator&) const
{
  return !(currentFragment == noFragments && currentBlock == noBlocks);
}
