#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;

#include "manager.hpp"
#include "types.hpp"
#include "tools.hpp"
#include "dehrust.hpp"
#include "lang.hpp"

#include "widestring.hpp"

extern PluginStartupInfo startupInfo;
extern Options op;

bool Manager::extractFile(int index, BYTE * buffer, int requested) {
  if(index < 0 || index >= noFiles || requested < 0)
    return false;
  size_t capacity = size_t(files[index].noSecs) * 256;
  size_t amount = requested ? size_t(requested) : capacity;
  if(amount > capacity)
    return false;
  speccy::Bytes output(capacity, 0);
  size_t offset = infos[index].offset, total = 0;
  for(;;) {
    if(offset > archive.size() || archive.size() - offset < sizeof(BlockHdr))
      return false;
    BlockHdr block{};
    memcpy(&block, archive.data() + offset, sizeof(block));
    size_t length = sizeof(block) + block.extInfoSize + block.pSize;
    if(length > archive.size() - offset || block.extInfoSize < sizeof(ExtInfo) ||
       memcmp(block.signature, "Hrst2", 5) || block.size > capacity - total)
      return false;
    ExtInfo info{};
    memcpy(&info, archive.data() + offset + sizeof(block), sizeof(info));
    const BYTE * packed = archive.data() + offset + sizeof(block) + block.extInfoSize;
    if(crc16calc(packed, block.pSize) != info.pCRC)
      return false;
    if(block.flags & 1) {
      if(block.pSize != block.size)
        return false;
      memcpy(output.data() + total, packed, block.size);
    } else {
      size_t written = 0;
      if(!dehrust(packed, block.pSize, output.data() + total, block.size, written) ||
         written != block.size)
        return false;
    }
    if(crc16calc(output.data() + total, block.size) != info.CRC)
      return false;
    total += block.size;
    offset += length;
    if(block.flags & 2)
      break;
  }
  if(total < fileSize(files[index]))
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
    getMsg(MDeHrust),     getMsg(MDeHrustButton), getMsg(MHoBeta),    getMsg(MSCL),
    getMsg(MSkipHeaders), getMsg(MCancel),        getMsg(MCRCError),  getMsg(MCanNotCreate),
    getMsg(MWarning),     getMsg(MAlreadyExists), getMsg(MOverwrite), getMsg(MOverwriteAll),
    getMsg(MSkip),        getMsg(MSkipAll),       getMsg(MOk)};
  return speccy::exportArchive(
    startupInfo, op.defaultFormat, "xHRiP_copy_path", messages, files, pcFiles, noFiles, items,
    count, move, path, mode,
    [this](int i, BYTE * output, int size) {
      return extractFile(i, output, size);
    },
    [](const FileHdr & header) {
      return fileSize(header);
    });
}
