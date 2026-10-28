#pragma once
#include "io.hpp"
#include <algorithm>

namespace speccy {
  inline bool zipContinuation(const uint8_t * header) {
    return !std::memcmp(header, "********ZIP", 11);
  }

  inline size_t hobetaBodySize(const Bytes & data) {
    size_t sectors = size_t(data[14]) * 256, logical = le16(data.data() + 11);
    return logical && logical <= sectors && sectors - logical < 256 ? logical : sectors;
  }

  inline bool zipVolumes(const std::string & main, Bytes & stream) {
    Bytes file;
    if(!readFile(main, file) || !hobeta(file.data(), file.size()) ||
       std::memcmp(file.data() + 8, "ZIP", 3) || zipContinuation(file.data()))
      return false;
    stream.assign(file.begin() + 17, file.begin() + 17 + hobetaBodySize(file));
    auto base = fileName(main);
    auto directory = parentPath(main);
    if(directory.empty())
      directory = ".";

    std::vector<std::string> used;
    for(size_t i = 0; i < 255; ++i) {
      std::string suffix = std::to_string(i);
      std::string next;

      for(const auto & folder : {directory, joinPath(directory, ".zxzip-volumes")}) {
        for(const auto & name : {base + suffix, base + std::string(i + 1, '0')}) {
          std::error_code ec;
          auto candidate = joinPath(folder, name);
          bool exists = fileExists(candidate, ec);
          if(ec)
            return false;
          if(exists) {
            next = candidate;
            break;
          }
        }
        if(!next.empty())
          break;
      }
      if(next.empty())
        break;
      if(std::find(used.begin(), used.end(), next) != used.end())
        return false;
      if(!readFile(next, file) || !hobeta(file.data(), file.size()) ||
         !zipContinuation(file.data()))
        return false;
      size_t size = hobetaBodySize(file);
      if(!size || stream.size() + size > 16 * 1024 * 1024)
        return false;
      stream.insert(stream.end(), file.begin() + 17, file.begin() + 17 + size);
      used.push_back(next);
    }
    return true;
  }

  inline bool hripVolumes(const std::string & main, size_t headerOffset, Bytes & stream) {
    Bytes file;
    if(!readFile(main, file) || !hobeta(file.data(), file.size()) || headerOffset > file.size())
      return false;
    size_t end = 17 + hobetaBodySize(file);
    if(headerOffset > end)
      return false;
    stream.assign(file.begin() + headerOffset, file.begin() + end);
    auto directory = parentPath(main);
    if(directory.empty())
      directory = ".";
    std::vector<std::string> entries, candidates;
    if(!listDirectory(directory, entries))
      return false;
    auto stem = fileStem(main);
    for(const auto & path : entries) {
      if(path == main || fileName(path).compare(0, stem.size(), stem))
        continue;
      candidates.push_back(path);
    }
    std::sort(candidates.begin(), candidates.end());
    for(unsigned i = 0; i < 15; ++i) {
      bool found = false;
      for(const auto & path : candidates) {
        if(!readFile(path, file, 65536 + 17) || !hobeta(file.data(), file.size()))
          continue;
        uint8_t type = i < 10 ? '0' + i : 'A' + i - 10;
        if(file[8] != type || file[9] != 'r' || file[10] != 'p')
          continue;
        size_t size = hobetaBodySize(file);
        if(!size)
          return false;
        stream.insert(stream.end(), file.begin() + 17, file.begin() + 17 + size);
        found = true;
        break;
      }
      if(!found)
        break;
    }
    return true;
  }
}
