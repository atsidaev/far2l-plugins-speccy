#ifndef DETECTORS_HPP
#define DETECTORS_HPP

#include "detector.hpp"

#define DETECTOR_DECL(name)                                                                                           \
	namespace name                                                                                                    \
	{                                                                                                                 \
		bool WINAPI detect (DetectorResult& result, const char *file_name, const HoHdr *hdr, const BYTE *data, int data_size);                 \
		bool WINAPI extract(HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD internal_id); \
	}

DETECTOR_DECL(ALASM)
DETECTOR_DECL(BASIC)
DETECTOR_DECL(CC3)
DETECTOR_DECL(DSQ)
DETECTOR_DECL(HRUM)
DETECTOR_DECL(HRUST1)
DETECTOR_DECL(HRUST21)
DETECTOR_DECL(HRUST22)
DETECTOR_DECL(LZS)
DETECTOR_DECL(MASM)
DETECTOR_DECL(MSPACK)
DETECTOR_DECL(PCD)
DETECTOR_DECL(STORM)
DETECTOR_DECL(TASM)
DETECTOR_DECL(TRACKERS)
DETECTOR_DECL(TRUSH)
DETECTOR_DECL(XAS)
DETECTOR_DECL(XGFX)
DETECTOR_DECL(ZXASM)

namespace BASIC { void WINAPI init(const char *path); }

const Detector static_detector[] = {
  {0, ALASM::detect, ALASM::extract, 0},       {BASIC::init, BASIC::detect, BASIC::extract, 0},
  {0, CC3::detect, CC3::extract, 0},           {0, DSQ::detect, DSQ::extract, 0},
  {0, HRUM::detect, HRUM::extract, 0},         {0, HRUST1::detect, HRUST1::extract, 0},
  {0, HRUST21::detect, HRUST21::extract, 0},   {0, HRUST22::detect, HRUST22::extract, 0},
  {0, LZS::detect, LZS::extract, 0},           {0, MASM::detect, MASM::extract, 0},
  {0, MSPACK::detect, MSPACK::extract, 0},     {0, PCD::detect, PCD::extract, 0},
  {0, STORM::detect, STORM::extract, 0},       {0, TASM::detect, TASM::extract, 0},
  {0, TRACKERS::detect, TRACKERS::extract, 0}, {0, TRUSH::detect, TRUSH::extract, 0},
  {0, XAS::detect, XAS::extract, 0},           {0, XGFX::detect, XGFX::extract, 0},
  {0, ZXASM::detect, ZXASM::extract, 0}};

const int no_static_detectors = sizeof(static_detector)/sizeof(static_detector[0]);

#endif
