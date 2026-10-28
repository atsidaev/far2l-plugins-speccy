#include "manager.hpp"
#include "detector.hpp"
#include "../shared/archive_names.hpp"
extern Detector * detector;
extern Options op;

void Manager::makePCNames() {
  speccy::archiveNames(files, pcFiles, noFiles, *detector, op.detectFormat,
                       [this](int i, BYTE * out, int size) {
                         return extractFile(i, out, size);
                       });
}
