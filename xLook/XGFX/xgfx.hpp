#pragma once
#include "../dtr.hpp"
#include "../../shared/bytes.hpp"
#include <vector>

namespace XGFX {
  bool decodePages(const HoHdr * hdr, const BYTE * data, size_t size,
                   std::vector<speccy::Bytes> & pages);
}
