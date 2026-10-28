#pragma once
#include "bytes.hpp"
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <string>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace speccy {
  inline std::string joinPath(const std::string & parent, const std::string & name) {
    if(parent.empty() || (!name.empty() && name[0] == '/'))
      return name;
    return parent + (parent.back() == '/' ? "" : "/") + name;
  }

  inline std::string parentPath(const std::string & path) {
    auto slash = path.find_last_of('/');
    return slash == std::string::npos ? "" : path.substr(0, slash ? slash : 1);
  }

  inline std::string fileName(const std::string & path) {
    auto slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
  }

  inline std::string fileExtension(const std::string & path) {
    auto name = fileName(path);
    auto dot = name.find_last_of('.');
    return dot == std::string::npos || dot == 0 || name == ".." ? "" : name.substr(dot);
  }

  inline std::string fileStem(const std::string & path) {
    auto name = fileName(path);
    return name.substr(0, name.size() - fileExtension(path).size());
  }

  inline bool fileExists(const std::string & path, std::error_code & ec) {
    struct stat status;
    ec.clear();
    if(stat(path.c_str(), &status) == 0)
      return true;
    if(errno != ENOENT && errno != ENOTDIR)
      ec.assign(errno, std::generic_category());
    return false;
  }

  inline bool fileExists(const std::string & path) {
    std::error_code ec;
    return fileExists(path, ec);
  }

  inline bool isRegularFile(const std::string & path) {
    struct stat status;
    return stat(path.c_str(), &status) == 0 && S_ISREG(status.st_mode);
  }

  inline bool createDirectories(const std::string & path) {
    if(path.empty())
      return true;
    struct stat status;
    if(stat(path.c_str(), &status) == 0)
      return S_ISDIR(status.st_mode);
    if(errno != ENOENT || !createDirectories(parentPath(path)))
      return false;
    if(mkdir(path.c_str(), 0777) == 0)
      return true;
    return errno == EEXIST && stat(path.c_str(), &status) == 0 && S_ISDIR(status.st_mode);
  }

  inline bool listDirectory(const std::string & path, std::vector<std::string> & entries) {
    entries.clear();
    DIR * directory = opendir(path.c_str());
    if(!directory)
      return false;
    for(;;) {
      errno = 0;
      dirent * entry = readdir(directory);
      if(!entry) {
        int error = errno;
        closedir(directory);
        return error == 0;
      }
      if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
        entries.push_back(joinPath(path, entry->d_name));
    }
  }

  inline void removeTree(const std::string & path) {
    struct stat status;
    if(lstat(path.c_str(), &status) != 0)
      return;
    if(S_ISDIR(status.st_mode)) {
      std::vector<std::string> entries;
      if(!listDirectory(path, entries))
        return;
      for(const auto & entry : entries)
        removeTree(entry);
      rmdir(path.c_str());
    } else {
      unlink(path.c_str());
    }
  }

  inline std::string temporaryDirectory() {
    const char * names[] = {"TMPDIR", "TMP", "TEMP", "TEMPDIR"};
    for(const char * name : names) {
      const char * value = getenv(name);
      if(value && *value)
        return value;
    }
    return "/tmp";
  }

  inline bool readFile(const std::string & name, Bytes & bytes, size_t limit = 16 * 1024 * 1024) {
    std::ifstream in(name.c_str(), std::ios::binary | std::ios::ate);
    if(!in)
      return false;
    auto size = in.tellg();
    if(size < 0 || uint64_t(size) > limit)
      return false;
    bytes.resize(size_t(size));
    in.seekg(0);
    return bytes.empty() || bool(in.read(reinterpret_cast<char *>(bytes.data()), bytes.size()));
  }

  class TempDirectory {
    std::string path_;

  public:
    TempDirectory() = default;
    TempDirectory(const TempDirectory &) = delete;
    TempDirectory & operator=(const TempDirectory &) = delete;

    ~TempDirectory() {
      clear();
    }

    void clear() {
      if(path_.empty())
        return;
      removeTree(path_);
      path_.clear();
    }

    bool create(const std::string & parent, const std::string & prefix) {
      clear();
      char * absolute = realpath(parent.c_str(), nullptr);
      if(!absolute)
        return false;
      std::string pattern = joinPath(absolute, prefix + "XXXXXX");
      free(absolute);
      if(!mkdtemp(&pattern[0]))
        return false;
      path_ = pattern;
      return true;
    }

    const std::string & path() const {
      return path_;
    }

    bool valid() const {
      return !path_.empty();
    }
  };

  inline bool writeFile(const std::string & target, const Bytes & data) {
    if(!createDirectories(parentPath(target)))
      return false;
    std::string temporary = joinPath(parentPath(target), ".speccy-XXXXXX");
    int fd = mkstemp(&temporary[0]);
    if(fd < 0)
      return false;
    bool ok = true;
    size_t offset = 0;
    while(offset < data.size()) {
      ssize_t n = ::write(fd, data.data() + offset, data.size() - offset);
      if(n < 0 && errno == EINTR)
        continue;
      if(n <= 0) {
        ok = false;
        break;
      }
      offset += size_t(n);
    }
    if(ok && fsync(fd) != 0)
      ok = false;
    if(close(fd) != 0)
      ok = false;
    if(ok && ::rename(temporary.c_str(), target.c_str()) != 0)
      ok = false;
    if(!ok)
      ::unlink(temporary.c_str());
    return ok;
  }

  inline std::string trdosName(const char * name) {
    std::string result;
    for(int i = 0; i < 8; ++i) {
      unsigned char c = name[i];
      result += c > 32 && c < 127 && std::strchr(".\\/:?*<>|", c) == nullptr ? char(c) : '_';
    }
    auto first = result.find_first_not_of('_'), last = result.find_last_not_of('_');
    return first == std::string::npos ? "________" : result.substr(first, last - first + 1);
  }
}
