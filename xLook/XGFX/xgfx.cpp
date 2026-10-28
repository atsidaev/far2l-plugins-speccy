#include "xgfx.hpp"
#include "image.hpp"
#include "formatFactory.hpp"
#include "format.hpp"
#include "../../shared/png.hpp"
#include <memory>
#include <limits>

namespace XGFX
{
static_assert(sizeof(theX::HoHdr) == sizeof(HoHdr), "HoBeta header layout mismatch");

static std::unique_ptr<theX::Image> decode(const HoHdr * hdr, const BYTE * data, size_t size) {
  auto * format =
    theX::Factory::get()->detect(reinterpret_cast<const theX::HoHdr *>(hdr), data, size);
  return std::unique_ptr<theX::Image>(
    format ? format->convert(reinterpret_cast<const theX::HoHdr *>(hdr), data, size) : nullptr);
}

static bool png(const theX::Page & page, speccy::Bytes & bytes) {
  return speccy::pngFromBgr(reinterpret_cast<const uint8_t *>(page.pix), page.w, page.h, bytes);
}

bool decodePages(const HoHdr * hdr, const BYTE * data, size_t size,
                 std::vector<speccy::Bytes> & pages) {
  auto image = decode(hdr, data, size);
  if(!image || !image->noPages || image->noPages > 256)
    return false;
  std::vector<speccy::Bytes> result(image->noPages);
  for(size_t i = 0; i < image->noPages; ++i)
    if(!image->getPage(i) || !png(*image->getPage(i), result[i]))
      return false;
  pages = std::move(result);
  return true;
}

bool WINAPI detect(DetectorResult & result, const char *, const HoHdr * hdr, const BYTE * data,
                   int size) {
  if(size < 0)
    return false;
  auto * format =
    theX::Factory::get()->detect(reinterpret_cast<const theX::HoHdr *>(hdr), data, size);
  if(!format)
    return false;
  strcpy(result.title, "[graphics]");
  strcpy(result.format, "theX");
  strcpy(result.ext, ".png");
  result.action = do_extract;
  return true;
}

bool WINAPI extract(HoHdr * hdr, const BYTE * data, DWORD size, BYTE * out, DWORD * capacity,
                    DWORD) {
  std::vector<speccy::Bytes> pages;
  if(!decodePages(hdr, data, size, pages))
    return false;
  auto needed = pages[0].size();
  if(needed > *capacity) {
    *capacity = needed;
    return false;
  }
  memcpy(out, pages[0].data(), needed);
  *capacity = needed;
  return true;
}
}
