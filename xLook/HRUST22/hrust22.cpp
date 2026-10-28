#include "../dtr.hpp"
#include "../../shared/hrust.hpp"

namespace HRUST22
{
  bool WINAPI extract(HoHdr *, const BYTE * source, DWORD sourceSize, BYTE * out, DWORD * size,
                      DWORD) {
    if(sourceSize < 11)
      return false;
    size_t header = 11 + source[10];
    size_t expected = speccy::le16(source + 6), packed = speccy::le16(source + 8);
    if(header > sourceSize || packed > sourceSize - header || expected > *size)
      return false;
    size_t written = expected;
    if(source[5] & 1) {
      if(packed < expected)
        return false;
      memcpy(out, source + header, expected);
    } else if(!speccy::hrust2(source + header, packed, out, *size, written) || written != expected)
      return false;
    *size = written;
    return true;
  }

  bool WINAPI detect(DetectorResult & result, const char *, const HoHdr *, const BYTE * data,
                     int size) {
    if(!(size >= 11 && !memcmp(data, "Hrst2", 5) && (data[5] & 0xDE) == 0x06))
      return false;
    size_t header = 11 + data[10];
    size_t packed = speccy::le16(data + 8);
    if(header > size_t(size) || packed > size_t(size) - header)
      return false;
    result.action = do_extract;
    strcpy(result.title, "[hrust 2.2+]");
    return true;
  }
}
