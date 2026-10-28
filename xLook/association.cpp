#include "association.hpp"
#include "../shared/io.hpp"
#include <sstream>
#include <sys/wait.h>

std::vector<Association> associations;

static std::string trim(std::string s) {
  auto a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
  return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

bool read_associations(const char * module) {
  associations.clear();
  speccy::Bytes bytes;
  if(!module || !speccy::readFile(speccy::joinPath(speccy::parentPath(module), "associat.ion"),
                                  bytes, 1024 * 1024))
    return false;
  std::istringstream stream(std::string(bytes.begin(), bytes.end()));
  Association current;
  auto save = [&]() {
    if(!current.format.empty() && (!current.view.empty() || !current.edit.empty()))
      associations.push_back(current);
  };
  for(std::string line; std::getline(stream, line);) {
    line = trim(line);
    if(line.empty() || line[0] == ';' || line[0] == '#')
      continue;
    if(line.size() > 4096 || associations.size() >= 256)
      return false;
    if(line.front() == '[' && line.back() == ']') {
      save();
      current = {};
      current.format = trim(line.substr(1, line.size() - 2));
      continue;
    }
    auto eq = line.find('=');
    if(eq == std::string::npos)
      continue;
    auto key = trim(line.substr(0, eq)), value = trim(line.substr(eq + 1));
    if(key == "view")
      current.view = value;
    if(key == "edit")
      current.edit = value;
  }
  save();
  return true;
}

bool viewerArguments(const std::string & command, const std::string & file,
                     std::vector<std::string> & args) {
  args.clear();
  std::string arg;
  char quote = 0;
  bool started = false;
  for(size_t i = 0; i < command.size(); ++i) {
    char c = command[i];
    if(c == '\\' && quote != '\'') {
      if(++i == command.size())
        return false;
      arg += command[i];
      started = true;
    } else if(c == '%' && i + 1 < command.size() && command[i + 1] == 'f') {
      arg += file;
      ++i;
      started = true;
    } else if(c == '\'' || c == '"') {
      if(!quote)
        quote = c;
      else if(quote == c)
        quote = 0;
      else
        arg += c;
      started = true;
    } else if(!quote && (c == ' ' || c == '\t')) {
      if(started) {
        args.push_back(arg);
        arg.clear();
        started = false;
      }
    } else {
      arg += c;
      started = true;
    }
  }
  if(quote)
    return false;
  if(started)
    args.push_back(arg);
  return !args.empty() && !args[0].empty();
}

bool startExternalViewer(const std::string & command, const std::string & file) {
  std::vector<std::string> args;
  if(!viewerArguments(command, file, args))
    return false;
  std::vector<char *> argv;
  for(auto & arg : args)
    argv.push_back(const_cast<char *>(arg.c_str()));
  argv.push_back(nullptr);
  int pipefd[2];
  if(pipe2(pipefd, O_CLOEXEC) != 0)
    return false;
  pid_t child = fork();
  if(child == 0) {
    close(pipefd[0]);
    pid_t detached = fork();
    if(detached > 0)
      _exit(0);
    if(detached == 0) {
      setsid();
      execvp(argv[0], argv.data());
    }
    int error = errno;
    while(write(pipefd[1], &error, sizeof(error)) < 0 && errno == EINTR) {
    }
    _exit(127);
  }
  close(pipefd[1]);
  if(child < 0) {
    close(pipefd[0]);
    return false;
  }
  int status = 0;
  while(waitpid(child, &status, 0) < 0 && errno == EINTR) {
  }
  int error = 0;
  ssize_t n;
  do {
    n = read(pipefd[0], &error, sizeof(error));
  } while(n < 0 && errno == EINTR);
  close(pipefd[0]);
  return n == 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
