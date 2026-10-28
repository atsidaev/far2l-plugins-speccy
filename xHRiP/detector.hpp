#pragma once
#include "types.hpp"
#include "../shared/formats.hpp"

class Detector
{
  speccy::Formats formats;

public:
  explicit Detector(const char * path);

  BYTE detect(const FileHdr & hdr, const BYTE * data, int size, char * comment) {
    return formats.detect(hdr, data, size < 0 ? 0 : size, comment);
  }

  void specialChar(BYTE i, char * p) {
    auto * r = formats.rule(i);
    if(r && r->special)
      *p = r->special;
  }

  void getType(BYTE i, char * p) {
    auto * r = formats.rule(i);
    if(r && r->newType)
      *p = r->newType;
  }

  bool getSkipHeader(BYTE i) {
    auto * r = formats.rule(i);
    return r && r->skipHeader;
  }

  const char * description(BYTE i) {
    auto * r = formats.rule(i);
    return r && !r->description.empty() ? r->description.c_str() : nullptr;
  }
};
