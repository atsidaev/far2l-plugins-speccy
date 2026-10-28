#pragma once
#include <string>
#include <vector>

struct Association {
  std::string format, view, edit;
};

extern std::vector<Association> associations;
bool read_associations(const char * module);

bool commandArguments(const std::string & command, std::vector<std::string> & args);

bool viewerArguments(const std::string & command, const std::string & file,
                     std::vector<std::string> & args);
bool startExternalViewer(const std::string & command, const std::string & file);
