#include "../dtr.hpp"
#include "../../shared/hrust.hpp"

namespace HRUST1
{
  bool WINAPI extract(HoHdr *, const BYTE * source, DWORD sourceSize, BYTE * out, DWORD * size,
                      DWORD) {
    size_t written = 0;
    if(!speccy::hrust1(source, sourceSize, out, *size, written))
      return false;
    *size = written;
    return true;
  }

  bool WINAPI detect(DetectorResult & result, const char *, const HoHdr *, const BYTE * data,
                     int size) {
    if(size < 12)
      return false;
    size_t offset = data[0] == 'H' && data[1] == 'R' ? 0 : 0x103;
    if(size_t(size) < offset + 12 || data[offset] != 'H' || data[offset + 1] != 'R')
      return false;
    if(!offset && data[2] == 'i' && (data[7] == 0 || data[7] == 1))
      return false;
    auto unpacked = speccy::le16(data + offset + 2), packed = speccy::le16(data + offset + 4);
    if(unpacked < 7 || packed > unpacked || packed > size_t(size) - offset)
      return false;
    result.action = do_extract;
    strcpy(result.title, "[hrust 1.x]");
    return true;
  }
}
