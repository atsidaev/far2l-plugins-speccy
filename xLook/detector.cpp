#include <windows.h>
#include "far2sdk/farplug-mb.h"
using namespace oldfar;
#include "detector.hpp"
#include "detectors.hpp"
#include "tools.hpp"
#include "../shared/io.hpp"

DetectorManager::DetectorManager(PluginStartupInfo info)
{
	no_detectors = 0;

        auto plugin_path = speccy::parentPath(info.ModuleName) + "/";

        for(int i = 0; i < no_static_detectors; ++i) {
          Detector & cur = detector[no_detectors];

          cur = static_detector[i];

          if(!cur.detect)
            continue;

          if(cur.init)
            cur.init(plugin_path.c_str());
          ++no_detectors;

          if(no_detectors == 256)
            break;
        }

        if(!no_detectors) {
          char * msgItems[] = {"xLook Warning", "No detectors loaded", "Ok"};
          info.Message(info.ModuleNumber, FMSG_WARNING, NULL, msgItems,
                       sizeof(msgItems) / sizeof(msgItems[0]), 1);
        }
}

DetectorManager::~DetectorManager()
{
	for(int i = 0; i < no_detectors; ++i)
	{
		Detector& cur = detector[i];

		if(cur.cleanup) cur.cleanup();
	}
}

Detector* DetectorManager::find(DetectorResult &result, char *name, const HoHdr *hdr, const BYTE *body, int body_size)
{
  for(int i = 0; i < no_detectors; ++i) {
    DetectorResult candidate{};
    candidate.structSize = sizeof(candidate);
    if(detector[i].detect(candidate, name, hdr, body, body_size)) {
      result = candidate;
      return &detector[i];
    }
  }

  return 0;
}
