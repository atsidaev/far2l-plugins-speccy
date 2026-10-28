#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "extract.hpp"
#include "lang.hpp"

#include "widestring.hpp"

#include "debug.hpp"

extern PluginStartupInfo startupInfo;
extern Options op;
extern WORD noBytesWritten;

bool Manager::extractFile(int index, BYTE * buffer, int requested) {
  if(index < 0 || index >= noFiles || requested < 0)
    return false;
  const auto & hdr = files[index];
  size_t capacity = size_t(hdr.noSecs) * 256, size = fileSize(hdr), offset = pcFiles[index].offset;
  size_t amount = requested ? size_t(requested) : capacity;
  if(amount > capacity || offset > archive.size() || hdr.packedSize > archive.size() - offset)
    return false;
  speccy::Bytes output(capacity, 0);
  BYTE * packed = archive.data() + offset;
  try {
    switch(hdr.pMethod) {
    case 0:
      if(hdr.packedSize != size)
        return false;
      memcpy(output.data(), packed, size);
      break;
    case 1:
      unlz(packed, hdr.packedSize, output.data(), size);
      break;
    case 2:
      unshrink(packed, hdr.packedSize, output.data(), size);
      break;
    case 3:
      explode(packed, hdr.packedSize, output.data(), size, size < 0x1600, hdr.flags & 1);
      break;
    default:
      return false;
    }
  } catch(const speccy::DecodeError &) {
    return false;
  }
  if(hdr.pMethod && noBytesWritten != size)
    return false;
  if(crc32(output.data(), size) != hdr.crc)
    return false;
  if(amount)
    memcpy(buffer, output.data(), amount);
  return true;
}

#include "../shared/archive_export.hpp"

int Manager::getFiles(PluginPanelItem * items, int count, int move, char * path, int mode) {
  if(!readInfo())
    return 0;
  speccy::ExportMessages messages = {
    getMsg(MExtract),     getMsg(MExtractButton), getMsg(MHoBeta),    getMsg(MSCL),
    getMsg(MSkipHeaders), getMsg(MCancel),        getMsg(MCRCError),  getMsg(MCanNotCreate),
    getMsg(MWarning),     getMsg(MAlreadyExists), getMsg(MOverwrite), getMsg(MOverwriteAll),
    getMsg(MSkip),        getMsg(MSkipAll),       getMsg(MOk)};
  return speccy::exportArchive(
    startupInfo, op.defaultFormat, "xZXZIP_copy_path", messages, files, pcFiles, noFiles, items,
    count, move, path, mode,
    [this](int i, BYTE * output, int size) {
      return extractFile(i, output, size);
    },
    [](const ZipedFileHdr & header) {
      return fileSize(header);
    });
}
