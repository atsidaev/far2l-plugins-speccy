#ifndef iterator_hpp
#define iterator_hpp

#include <windows.h>
#include "types.hpp"
#include "FmtPlugin.hpp"
#include "iSDOS.hpp"

class Iterator
{
  public:
    Iterator(FmtPlugin* filer, HANDLE img, UniHdr h);
    Iterator();
    ~Iterator();
    
    const int operator*  () const;
    Iterator& operator++ ();
    bool operator== (const Iterator&) const;
    bool operator!= (const Iterator&) const;
    
  private:
    u16 noFragments;
    u16 startBlock;
    u16 noBlocks;

    u16 currentBlock;
    u16 currentFragment;

    u8* fragmentsList;
};

#endif
