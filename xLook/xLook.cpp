#include "xLook.hpp"
#include "detector.hpp"
#include "association.hpp"
#include "tools.hpp"
#include "XGFX/xgfx.hpp"
#include "../shared/io.hpp"
#include <memory>
#include <algorithm>

PluginStartupInfo startup_info{};
FarStandardFunctions FSF{};

namespace {
  using speccy::Bytes;

  std::unique_ptr<DetectorManager> manager;
  speccy::TempDirectory tempRoot;
  std::vector<std::unique_ptr<speccy::TempDirectory>> viewerDirectories;

  struct DecodedFile {
    std::string host, title, base, extension, format;
    std::vector<Bytes> pages;

    std::string pageName(size_t page) const {
      return base + (page ? "_" + std::to_string(page + 1) : "") + extension;
    }
  };

  void error(const char * text) {
    const char * items[] = {"xLook", text, "OK"};
    startup_info.Message(startup_info.ModuleNumber, FMSG_WARNING, nullptr, items, 3, 1);
  }

  std::unique_ptr<DecodedFile> decode(const char * name, const Bytes & input, bool allowRawHoBeta) {
    if(!manager || !name || input.size() < 17 || input.size() > 16 * 1024 * 1024)
      return nullptr;
    HoHdr header{};
    const HoHdr * hdr = nullptr;
    if(speccy::hobeta(input.data(), input.size())) {
      memcpy(&header, input.data(), 17);
      hdr = &header;
    }
    size_t offset = hdr ? 17 : 0, size = input.size() - offset;

    Bytes padded(input.begin() + offset, input.end());
    padded.resize(size + 65536, 0);
    DetectorResult result{};
    result.structSize = sizeof(result);
    Detector * detector = manager->find(result, const_cast<char *>(name), hdr, padded.data(), size);
    bool extracting = detector && result.action == do_extract && detector->extract;
    if(!extracting && !(hdr && allowRawHoBeta))
      return nullptr;
    std::unique_ptr<DecodedFile> file(new DecodedFile);
    file->host = name;
    char base[9]{};
    if(hdr)
      memcpy(base, hdr->name, 8);
    else {
      auto stem = speccy::fileStem(name);
      memcpy(base, stem.data(), std::min(stem.size(), size_t(8)));
    }
    file->base = speccy::trdosName(base);
    file->extension = extracting && result.ext[0] ? result.ext : ".bin";
    file->format = extracting ? result.format : "";
    file->title = " xLook:" + speccy::fileName(file->host) + " " +
                  (result.title[0] ? result.title : "[HoBeta]") + " ";
    if(extracting && file->format == "theX") {
      if(!XGFX::decodePages(hdr, padded.data(), size, file->pages))
        return nullptr;
    } else if(extracting) {
      Bytes out(65536, 0);
      DWORD written = out.size();
      bool ok = detector->extract(const_cast<HoHdr *>(hdr), padded.data(), size, out.data(),
                                  &written, result.internal_id);
      if(!ok && written > out.size() && written <= 16 * 1024 * 1024) {
        out.resize(written);
        ok = detector->extract(const_cast<HoHdr *>(hdr), padded.data(), size, out.data(), &written,
                               result.internal_id);
      }
      if(!ok || written > out.size())
        return nullptr;
      out.resize(written);
      file->pages.push_back(std::move(out));
    } else {
      size_t length = size;
      if(hdr->noSex && hdr->length >= 256u * (hdr->noSex - 1) && hdr->length <= size)
        length = hdr->length;
      file->pages.emplace_back(padded.begin(), padded.begin() + length);
    }
    return file;
  }
}

void WINAPI SetStartupInfo(const PluginStartupInfo * info) {
  startup_info = *info;
  FSF = *info->FSF;
  startup_info.FSF = &FSF;
  manager.reset(new DetectorManager(startup_info));
  read_associations(info->ModuleName);
}

void WINAPI ExitFAR() {
  viewerDirectories.clear();
  tempRoot.clear();
  manager.reset();
}

void WINAPI GetPluginInfo(PluginInfo * info) {
  *info = {};
  info->StructSize = sizeof(*info);
  info->Flags = PF_FULLCMDLINE;
  info->CommandPrefix = "xv:xe";
}

void WINAPI GetOpenPluginInfo(HANDLE handle, OpenPluginInfo * info) {
  auto & file = *static_cast<DecodedFile *>(handle);
  *info = {};
  info->StructSize = sizeof(*info);
  info->Flags = OPIF_ADDDOTS;
  info->HostFile = file.host.c_str();
  info->PanelTitle = file.title.c_str();
  info->CurDir = "";
}

int WINAPI GetFindData(HANDLE handle, PluginPanelItem ** items, int * count, int) {
  auto & file = *static_cast<DecodedFile *>(handle);
  *count = file.pages.size();
  *items = new PluginPanelItem[*count]{};
  for(int i = 0; i < *count; ++i) {
    auto & item = (*items)[i];
    auto name = file.pageName(i);
    snprintf(item.FindData.cFileName, sizeof(item.FindData.cFileName), "%s", name.c_str());
    item.FindData.nFileSize = file.pages[i].size();
    item.UserData = i;
  }
  return TRUE;
}

void WINAPI FreeFindData(HANDLE, PluginPanelItem * items, int) {
  delete[] items;
}

int WINAPI GetFiles(HANDLE handle, PluginPanelItem * items, int count, int move, char * destination,
                    int mode) {
  if(count < 1 || move || !destination)
    return 0;
  auto & file = *static_cast<DecodedFile *>(handle);
  std::string path = destination;
  if(!(mode & OPM_SILENT)) {
    FarDialogItem dialog[6];
    dialog[0] = InitItem(DI_DOUBLEBOX, 3, 1, 52, 6, 0, nullptr, 0, 0, "Copy");
    dialog[1] = InitItem(DI_TEXT, 5, 2, 0, 0, 0, nullptr, 0, 0, "Copy to:");
    dialog[2] = InitItem(DI_EDIT, 5, 3, 50, 0, 1, "xLook_export_to", DIF_HISTORY | DIF_EDITEXPAND,
                         0, path.c_str());
    dialog[3] = InitItem(DI_TEXT, 0, 4, 0, 0, 0, nullptr, DIF_SEPARATOR, 0, nullptr);
    dialog[4] = InitItem(DI_BUTTON, 11, 5, 0, 0, 0, nullptr, DIF_CENTERGROUP, 1, "&OK");
    dialog[5] = InitItem(DI_BUTTON, 35, 5, 0, 0, 0, nullptr, DIF_CENTERGROUP, 0, "&Cancel");
    if(startup_info.Dialog(startup_info.ModuleNumber, -1, -1, 56, 8, nullptr, dialog, 6) != 4)
      return -1;
    path = dialog[2].Data;
  }
  if(path.empty())
    path = ".";
  for(int i = 0; i < count; ++i) {
    size_t page = items[i].UserData;
    if(page >= file.pages.size())
      return 0;
    auto target = speccy::joinPath(path, file.pageName(page));
    std::error_code ec;
    if(speccy::fileExists(target, ec)) {
      std::string name = target;
      const char * msg[] = {"xLook", "File already exists", name.c_str(), "Overwrite", "Cancel"};
      if(startup_info.Message(startup_info.ModuleNumber, FMSG_WARNING, nullptr, msg, 5, 2) != 0)
        return -1;
    }
    if(!speccy::writeFile(target, file.pages[page])) {
      error("Cannot write file");
      return 0;
    }
    items[i].Flags &= ~PPIF_SELECTED;
  }
  return 1;
}

HANDLE WINAPI OpenFilePlugin(char * name, const unsigned char * data, int size) {
  if(!name || !data || size < 17)
    return INVALID_HANDLE_VALUE;
  try {
    Bytes input;
    if(!speccy::readFile(name, input))
      return INVALID_HANDLE_VALUE;
    auto file = decode(name, input, false);
    return file ? file.release() : INVALID_HANDLE_VALUE;
  } catch(const std::exception &) {
    return INVALID_HANDLE_VALUE;
  }
}

void WINAPI ClosePlugin(HANDLE handle) {
  delete static_cast<DecodedFile *>(handle);
}

HANDLE WINAPI OpenPlugin(int from, INT_PTR item) {
  if(from != OPEN_COMMANDLINE || !item)
    return INVALID_HANDLE_VALUE;
  std::string command = reinterpret_cast<const char *>(item);
  auto colon = command.find(':');
  if(colon == std::string::npos)
    return INVALID_HANDLE_VALUE;
  auto prefix = command.substr(0, colon);
  if(prefix != "xv" && prefix != "xe")
    return INVALID_HANDLE_VALUE;
  std::vector<std::string> args;
  if(!commandArguments(command.substr(colon + 1), args) || args.size() != 1) {
    error("Specify one filename, quoting or escaping special characters");
    return INVALID_HANDLE_VALUE;
  }
  const auto & path = args[0];
  bool view = prefix == "xv";
  auto open = [&](const std::string & name, const char * title, int flags) {
    if(view)
      startup_info.Viewer(name.c_str(), title, 0, 0, -1, -1, flags);
    else
      startup_info.Editor(name.c_str(), title, 0, 0, -1, -1, flags, -1, -1);
  };
  try {
    Bytes input;
    if(!speccy::readFile(path, input)) {
      open(path, nullptr, 0);
      return INVALID_HANDLE_VALUE;
    }
    auto file = decode(path.c_str(), input, true);
    if(!file) {
      open(path, nullptr, 0);
      return INVALID_HANDLE_VALUE;
    }
    if(!tempRoot.valid()) {
      if(!tempRoot.create(speccy::temporaryDirectory(), "xlook-")) {
        error("Cannot create temporary directory");
        return INVALID_HANDLE_VALUE;
      }
    }
    std::unique_ptr<speccy::TempDirectory> session(new speccy::TempDirectory);
    if(!session->create(tempRoot.path(), "view-")) {
      error("Cannot create temporary directory");
      return INVALID_HANDLE_VALUE;
    }
    auto firstPage = speccy::joinPath(session->path(), file->pageName(0));
    for(size_t i = 0; i < file->pages.size(); ++i)
      if(!speccy::writeFile(speccy::joinPath(session->path(), file->pageName(i)), file->pages[i])) {
        error("Cannot write temporary file");
        return INVALID_HANDLE_VALUE;
      }
    viewerDirectories.push_back(std::move(session));
    for(const auto & association : associations) {
      if(association.format == file->format &&
         startExternalViewer(view ? association.view : association.edit, firstPage))
        return INVALID_HANDLE_VALUE;
    }
    open(firstPage, file->title.c_str(),
         view ? VF_DISABLEHISTORY | VF_NONMODAL | VF_ENABLE_F6 | VF_DELETEONLYFILEONCLOSE
              : EF_DISABLEHISTORY | EF_NONMODAL | EF_ENABLE_F6 | EF_DELETEONLYFILEONCLOSE);
  } catch(const std::exception & e) {
    error(e.what());
  }
  return INVALID_HANDLE_VALUE;
}
