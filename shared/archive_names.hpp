#pragma once
#include "io.hpp"
#include <set>
#include <cctype>

namespace speccy {
  inline std::string fold(std::string s) {
    for(char & c : s)
      c = std::tolower(static_cast<unsigned char>(c));
    return s;
  }

  template <class Header, class PanelEntry, class Detector, class Extract>
  void archiveNames(const Header * files, PanelEntry * pc, int count, Detector & detector,
                    bool detect, Extract extract) {
    std::set<std::string> used;
    std::string zipHead;
    unsigned part = 0;
    for(int i = 0; i < count; ++i) {
      auto & item = pc[i];
      item.comment[0] = 0;
      item.type = 255;
      item.skipHeader = false;
      std::string name = trdosName(files[i].name) + ".$";
      unsigned char type = files[i].type;
      name += type > 32 && type < 127 && !std::strchr(".\\/:?*<>|", type) ? char(type) : '_';
      size_t typePos = name.size() - 1;
      if(detect && files[i].noSecs) {
        Bytes bytes(size_t(files[i].noSecs) * 256);
        if(extract(i, bytes.data(), 0))
          item.type = detector.detect(files[i], bytes.data(), std::min(bytes.size(), size_t(4096)),
                                      item.comment);
      }
      if(item.type != 255) {
        detector.specialChar(item.type, &name[typePos - 1]);
        if(name.size() >= 3 && name[name.size() - 3] == '.')
          detector.getType(item.type, &name[typePos]);
        item.skipHeader = detector.getSkipHeader(item.type);
      }
      bool zip = !memcmp(&files[i].type, "ZIP", 3);
      if(zip && !memcmp(files[i].name, "********", 8) && !zipHead.empty())
        name = zipHead + std::to_string(part++);
      else if(zip) {
        zipHead = name;
        part = 0;
      } else
        zipHead.clear();
      std::string base = name;
      for(unsigned duplicate = 0; used.count(fold(name)); ++duplicate)
        name = base + std::to_string(duplicate);
      used.insert(fold(name));
      snprintf(item.name, sizeof(item.name), "%s", name.c_str());
    }
  }
}
