#include "detector.hpp"
#include "tools.hpp"
#include "lang.hpp"
extern Options op;

Detector::Detector(const char * path) {
  using namespace speccy;
  auto directory = parentPath(path);
  std::string candidates[] = {op.iniFilePath, joinPath(directory, "types.ini"),
                              joinPath(parentPath(directory), "types.ini")};
  for(const auto & file : candidates) {
    if(file.empty() || !isRegularFile(file))
      continue;
    if(!formats.load(file))
      errorMessage(getMsg(MINIError));
    return;
  }
  errorMessage(getMsg(MCanNotOpenINI));
}
