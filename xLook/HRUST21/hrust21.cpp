#include "../dtr.hpp"
#include "../../shared/hrust.hpp"

namespace HRUST21
{
  bool WINAPI extract(HoHdr *, const BYTE * source, DWORD sourceSize, BYTE * out, DWORD * size,
                      DWORD) {
    if(sourceSize < 8)
      return false;
    size_t header = 8;
    size_t expected = speccy::le16(source + 4), packed = speccy::le16(source + 6);
    if(header > sourceSize || packed > sourceSize - header || expected > *size)
      return false;
    size_t written = expected;
    if(source[3] & 0x80) {
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
    if(!(size >= 8 && !memcmp(data, "hr2", 3)))
      return false;
    size_t header = 8;
    size_t packed = speccy::le16(data + 6);
    if(header > size_t(size) || packed > size_t(size) - header)
      return false;
    result.action = do_extract;
    strcpy(result.title, "[hrust 2.1]");
    return true;
  }
}
