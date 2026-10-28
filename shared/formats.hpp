#pragma once
#include "io.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace speccy {
  struct FormatRule {
    int type = -1, size = -1, start = -1, sectors = -1;
    size_t signatureOffset = 0, commentOffset = 0, commentSize = 0;
    std::vector<int> signature;
    uint8_t special = 0, newType = 0;
    bool skipHeader = false;
    std::string description;
  };

  class Formats {
    std::vector<FormatRule> rules_;

    static std::string trim(const std::string & s) {
      auto a = s.find_first_not_of(" \r\t"), b = s.find_last_not_of(" \r\t");
      return a == std::string::npos ? "" : s.substr(a, b - a + 1);
    }

    static bool number(const std::string & s, int & n, int maximum = 65535) {
      if(s.empty())
        return false;
      if(s.size() >= 3 && s.front() == '\'' && s.back() == '\'' && s.size() <= 4) {
        n = uint8_t(s[1]);
        if(s.size() == 4)
          n |= uint8_t(s[2]) << 8;
      } else {
        std::string value = s;
        int base = 10;
        if(value[0] == '#') {
          value.erase(0, 1);
          base = 16;
        } else if(value.size() > 2 && value.substr(0, 2) == "0x") {
          value.erase(0, 2);
          base = 16;
        }
        try {
          size_t end = 0;
          n = std::stoi(value, &end, base);
          if(end != value.size())
            return false;
        } catch(...) {
          return false;
        }
      }
      return n >= 0 && n <= maximum;
    }

    static bool token(std::istringstream & in, std::string & value) {
      in >> std::ws;
      if(in.peek() == '\'') {
        in.get();
        value = "'";
        for(char c; in.get(c);) {
          value += c;
          if(c == '\'')
            return true;
        }
        return false;
      }
      return bool(in >> value);
    }

  public:
    bool load(const std::string & file) {
      rules_.clear();
      Bytes data;
      if(!readFile(file, data, 1024 * 1024))
        return false;
      std::istringstream stream(std::string(data.begin(), data.end()));
      FormatRule rule;
      bool valid = true, allValid = true;
      auto save = [&]() {
        if(valid && rules_.size() < 255 &&
           (rule.type >= 0 || rule.size >= 0 || rule.start >= 0 || rule.sectors >= 0 ||
            !rule.signature.empty()) &&
           (rule.special || rule.newType || rule.commentSize || !rule.description.empty()))
          rules_.push_back(rule);
        allValid &= valid;
      };
      for(std::string line; std::getline(stream, line);) {
        line = trim(line);
        if(line.empty() || line[0] == '#' || line[0] == ';')
          continue;
        if(line.size() > 4096) {
          valid = false;
          continue;
        }
        if(line[0] == '[') {
          save();
          rule = {};
          valid = true;
          continue;
        }
        auto eq = line.find('=');
        if(eq == std::string::npos) {
          valid = false;
          continue;
        }
        std::string key = trim(line.substr(0, eq)), value = trim(line.substr(eq + 1));
        int n = 0;
        if(key == "Type" || key == "Size" || key == "Start" || key == "NoSecs" ||
           key == "SpecialChar" || key == "NewType") {
          if(!number(value, n, (key == "Size" || key == "Start") ? 65535 : 255)) {
            valid = false;
            continue;
          }
          if(key == "Type")
            rule.type = n;
          if(key == "Size")
            rule.size = n;
          if(key == "Start")
            rule.start = n;
          if(key == "NoSecs")
            rule.sectors = n;
          if(key == "SpecialChar")
            rule.special = n;
          if(key == "NewType")
            rule.newType = n;
        } else if(key == "Description")
          rule.description = value;
        else if(key == "ShowHeader") {
          std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return std::tolower(c);
          });
          rule.skipHeader = value == "no";
        } else if(key == "Signature" || key == "Comment") {
          std::istringstream parts(value);
          std::string t;
          if(!token(parts, t) || !number(t, n)) {
            valid = false;
            continue;
          }
          if(key == "Comment") {
            rule.commentOffset = n;
            if(!token(parts, t) || !number(t, n, 255)) {
              valid = false;
              continue;
            }
            rule.commentSize = n;
          } else {
            rule.signatureOffset = n;
            rule.signature.clear();
            while(token(parts, t)) {
              if(t == "?")
                n = -1;
              else if(!number(t, n, 255)) {
                valid = false;
                break;
              }
              rule.signature.push_back(n);
            }
            if(rule.signature.empty() || rule.signature.size() > 32)
              valid = false;
          }
        }
      }
      save();
      return allValid;
    }

    const FormatRule * rule(uint8_t index) const {
      return index < rules_.size() ? &rules_[index] : nullptr;
    }

    template <class Header>
    uint8_t detect(const Header & hdr, const uint8_t * bytes, size_t size, char * comment) const {
      comment[0] = 0;
      for(size_t i = 0; i < rules_.size(); ++i) {
        const auto & r = rules_[i];
        if((r.type >= 0 && uint8_t(hdr.type) != r.type) || (r.size >= 0 && hdr.size != r.size) ||
           (r.start >= 0 && hdr.start != r.start) || (r.sectors >= 0 && hdr.noSecs != r.sectors))
          continue;
        if(r.signatureOffset > size || r.signature.size() > size - r.signatureOffset)
          continue;
        bool match = true;
        for(size_t j = 0; j < r.signature.size(); ++j)
          if(r.signature[j] >= 0 && bytes[r.signatureOffset + j] != r.signature[j]) {
            match = false;
            break;
          }
        if(!match)
          continue;
        if(r.commentOffset <= size && r.commentSize <= size - r.commentOffset) {
          std::string text;
          for(size_t j = 0; j < std::min(r.commentSize, size_t(99)); ++j) {
            uint8_t c = bytes[r.commentOffset + j];
            if(c > 32 && c < 127)
              text += char(c);
            else if(!text.empty() && text.back() != ' ')
              text += ' ';
          }
          text = trim(text);
          memcpy(comment, text.c_str(), text.size() + 1);
        }
        return uint8_t(i);
      }
      return 255;
    }
  };
}
