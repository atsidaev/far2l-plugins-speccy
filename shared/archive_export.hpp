#pragma once
#include "io.hpp"
#include <windows.h>
#include "far2sdk/farplug-mb.h"

namespace speccy {
  struct ExportMessages {
    const char * title, * button, * hobeta, * scl, * skipHeaders, * cancel, * crc, * writeError, * warning,
      * exists, * overwrite, * overwriteAll, * skip, * skipAll, * ok;
  };

  inline oldfar::FarDialogItem dialogItem(int type, int x1, int y1, int x2, int y2, int focus,
                                          const char * history, DWORD flags, int def,
                                          const char * text) {
    oldfar::FarDialogItem item{};
    item.Type = type;
    item.X1 = x1;
    item.Y1 = y1;
    item.X2 = x2;
    item.Y2 = y2;
    item.Focus = focus;
    item.Reserved = reinterpret_cast<DWORD_PTR>(history);
    item.Flags = flags;
    item.DefaultButton = def;
    snprintf(item.Data, sizeof(item.Data), "%s", text ? text : "");
    return item;
  }

  template <class Header, class PanelEntry, class Extract, class LogicalSize>
  int exportArchive(oldfar::PluginStartupInfo & info, int defaultFormat, const char * history,
                    const ExportMessages & msg, const Header * headers, const PanelEntry * entries,
                    int entryCount, oldfar::PluginPanelItem * selected, int count, int move,
                    const char * destination, int mode, Extract extract, LogicalSize logicalSize) {
    using namespace oldfar;
    if(count < 1)
      return 1;
    if(move || count > 255 || !destination)
      return 0;
    std::string path = destination;
    bool single = false, strip = false;
    if(!(mode & OPM_SILENT)) {
      FarDialogItem d[11];
      d[0] = dialogItem(DI_DOUBLEBOX, 3, 1, 72, 11, 0, nullptr, 0, 0, msg.title);
      d[1] = dialogItem(DI_TEXT, 5, 2, 0, 0, 0, nullptr, 0, 0, "Extract to:");
      d[2] =
        dialogItem(DI_EDIT, 5, 3, 70, 3, 1, history, DIF_HISTORY | DIF_EDITEXPAND, 0, path.c_str());
      d[3] = dialogItem(DI_TEXT, 3, 4, 0, 0, 0, nullptr, DIF_SEPARATOR, 0, nullptr);
      d[4] = dialogItem(DI_RADIOBUTTON, 5, 5, 0, 0, 0, nullptr, DIF_GROUP, 0, msg.hobeta);
      d[5] = dialogItem(DI_RADIOBUTTON, 5, 6, 0, 0, 0, nullptr, 0, 0, msg.scl);
      d[6] = dialogItem(DI_TEXT, 3, 7, 0, 0, 0, nullptr, DIF_SEPARATOR, 0, nullptr);
      d[7] = dialogItem(DI_CHECKBOX, 5, 8, 0, 0, 0, nullptr, 0, 0, msg.skipHeaders);
      d[8] = dialogItem(DI_TEXT, 3, 9, 0, 0, 0, nullptr, DIF_SEPARATOR, 0, nullptr);
      d[9] = dialogItem(DI_BUTTON, 0, 10, 0, 0, 0, nullptr, DIF_CENTERGROUP, 1, msg.button);
      d[10] = dialogItem(DI_BUTTON, 0, 10, 0, 0, 0, nullptr, DIF_CENTERGROUP, 0, msg.cancel);
      single = defaultFormat == 2 || (defaultFormat != 1 && count > 1);
      d[single ? 5 : 4].Selected = 1;
      if(info.Dialog(info.ModuleNumber, -1, -1, 76, 13, nullptr, d, 11) != 9)
        return -1;
      path = d[2].Data;
      single = d[5].Selected;
      strip = d[7].Selected;
    }
    if(path.empty())
      path = ".";
    auto error = [&](const char * text, const std::string & file) {
      const char * items[] = {msg.warning, text, file.c_str(), msg.ok};
      info.Message(info.ModuleNumber, FMSG_WARNING, nullptr, items, 4, 1);
    };

    std::vector<int> indices;
    std::vector<Bytes> bodies;
    for(int i = 0; i < count; ++i) {
      int index = 0;
      while(index < entryCount && std::strcmp(selected[i].FindData.cFileName, entries[index].name))
        ++index;
      if(index == entryCount)
        return 0;
      Bytes body(size_t(headers[index].noSecs) * 256, 0);
      if(!extract(index, body.data(), 0)) {
        error(msg.crc, entries[index].name);
        return 0;
      }
      if(count == 1 && (mode & (OPM_VIEW | OPM_EDIT)) && entries[index].skipHeader)
        strip = true;
      indices.push_back(index);
      bodies.push_back(std::move(body));
    }
    int overwriteAction = 0;
    auto save = [&](const std::string & target, const Bytes & bytes) -> int {
      std::error_code ec;
      bool exists = fileExists(target, ec);
      if(ec) {
        error(msg.writeError, target);
        return 0;
      }
      if(exists) {
        if(overwriteAction == 2)
          return 0;
        if(!overwriteAction) {
          auto name = target;
          const char * items[] = {msg.warning,      msg.exists, name.c_str(), msg.overwrite,
                                  msg.overwriteAll, msg.skip,   msg.skipAll,  msg.cancel};
          int answer = info.Message(info.ModuleNumber, FMSG_WARNING, nullptr, items, 8, 5);
          if(answer < 0 || answer == 4)
            return -1;
          if(answer == 3) {
            overwriteAction = 2;
            return 0;
          }
          if(answer == 2)
            return 0;
          if(answer == 1)
            overwriteAction = 1;
        }
      }
      if(!writeFile(target, bytes)) {
        error(msg.writeError, target);
        return 0;
      }
      return 1;
    };
    if(single) {
      Bytes output;
      if(!strip) {
        const char signature[] = "SINCLAIR";
        output.insert(output.end(), signature, signature + 8);
        output.push_back(count);
        for(int index : indices) {
          const auto * p = reinterpret_cast<const uint8_t *>(&headers[index]);
          output.insert(output.end(), p, p + 14);
        }
      }
      for(const auto & body : bodies)
        output.insert(output.end(), body.begin(), body.end());
      if(!strip)
        put32(output, sum(output.data(), output.size()));
      auto name = fileStem(entries[indices[0]].name) + (strip ? ".bin" : ".scl");
      int result = save(joinPath(path, name), output);
      if(result == 1)
        for(int i = 0; i < count; ++i)
          selected[i].Flags &= ~PPIF_SELECTED;
      return result;
    }
    int result = 1;
    for(int i = 0; i < count; ++i) {
      int index = indices[i];
      Bytes output;
      if(!strip) {
        const auto * h = reinterpret_cast<const uint8_t *>(&headers[index]);
        output.insert(output.end(), h, h + 13);
        output.push_back(0);
        output.push_back(headers[index].noSecs);
        put16(output, hobetaChecksum(output.data()));
      }
      size_t size = strip ? logicalSize(headers[index]) : bodies[i].size();
      if(size > bodies[i].size())
        return 0;
      output.insert(output.end(), bodies[i].begin(), bodies[i].begin() + size);
      int saved =
        save(joinPath(path, std::string(entries[index].name) + (strip ? ".bin" : "")), output);
      if(saved < 0)
        return -1;
      if(saved == 1)
        selected[i].Flags &= ~PPIF_SELECTED;
      else
        result = 0;
    }
    return result;
  }
}
