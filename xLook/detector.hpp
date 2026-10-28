#ifndef DETECTOR_HPP
#define DETECTOR_HPP

#include "common.hpp"

typedef void (WINAPI *init_t)      (const char *path);
typedef bool (WINAPI *detect_t)    (DetectorResult& result, const char *file_name, const HoHdr *hdr, const BYTE *data, int data_size);
typedef bool (WINAPI *extract_t)   (HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD internal_id);
//typedef bool (WINAPI *quickView_t) (HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD internal_id);
typedef void (WINAPI *cleanup_t)   (void);
struct Detector
{
	init_t      init;
	detect_t    detect;
	extract_t   extract;
//	quickView_t quickView;
	cleanup_t   cleanup;

};

class DetectorManager
{
public:
	DetectorManager(PluginStartupInfo info);
	~DetectorManager();
	Detector* find(DetectorResult &result, char *name, const HoHdr *hdr, const BYTE *body, int body_size);

private:
	int no_detectors;
	Detector detector[256];
};

#endif
