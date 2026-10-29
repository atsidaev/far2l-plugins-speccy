.DEFAULT_GOAL := all
# Latest v* tag (v0.9 → 0.9); override with `make VERSION=1.0 …`.
VERSION ?= $(shell git tag -l 'v*' --sort=-v:refname 2>/dev/null | head -n1 | sed 's/^v//')
ifeq ($(strip $(VERSION)),)
VERSION := 0.9
endif
FAR_PATH ?= far2l

CXXFLAGS = -c -fPIC 
CXXFLAGS += -Ishared -I$(FAR_PATH)/WinPort -I$(FAR_PATH)/far2l/include -I$(FAR_PATH)/far2l/
CXXFLAGS += -DWINPORT_DIRECT -DWINPORT_REGISTRY -DUNICODE

LD=g++
LDFLAGS = -shared

TARGETS = xCreate.far-plug-mb xSCL.far-plug-mb xTRD.far-plug-mb xISD.far-plug-mb xHRiP.far-plug-mb xZXZIP.far-plug-mb xLook.far-plug-mb
RELEASE = release/usr/lib/far2l/Plugins

SHARED_BASIC_OBJS = shared/registry.o shared/widestring.o
SHARED_OBJS = $(SHARED_BASIC_OBJS) shared/debug.o

XCREATE_OBJS = xCreate/xCreate.o xCreate/tools.o xCreate/creator.o \
               xCreate/Make_fdd.o xCreate/Make_fdi.o xCreate/Make_scl.o \
               xCreate/Make_td.o xCreate/Make_trd.o xCreate/Make_udi.o

XTRD_OBJS = xTRD/xTRD.o xTRD/manager.o xTRD/detector.o xTRD/FmtReader.o xTRD/tools.o \
            xTRD/mngr_tools.o xTRD/mngr_dir.o xTRD/mngr_del.o xTRD/mngr_get.o xTRD/mngr_put.o xTRD/mngr_key.o  \
            xTRD/Formats/TRD/trd.o xTRD/Formats/FDD/fdd.o xTRD/Formats/FDD/filer.o \
            xTRD/Formats/FDI/fdi.o xTRD/Formats/FDI/filer.o xTRD/Formats/TELEDISK/teledisk.o \
            xTRD/Formats/TELEDISK/filer.o xTRD/Formats/TELEDISK/td_tools.o \
            xTRD/Formats/UDI/udi.o xTRD/Formats/UDI/filer.o

XSCL_OBJS = xSCL/xSCL.o xSCL/manager_del.o xSCL/manager_get.o xSCL/manager_key.o xSCL/manager_put.o \
            xSCL/manager_tools.o xSCL/manager.o xSCL/tools.o xSCL/detector.o

XHRIP_OBJS = xHRiP/xHRiP.o xHRiP/manager.o xHRiP/manager_tools.o xHRiP/getFiles.o \
             xHRiP/tools.o xHRiP/detector.o xHRiP/dehrust.o

XZXZIP_OBJS = xZXZIP/xZXZIP.o xZXZIP/manager.o xZXZIP/manager_tools.o xZXZIP/getFiles.o \
              xZXZIP/tools.o xZXZIP/detector.o \
              xZXZIP/extract.o xZXZIP/extract1.o xZXZIP/extract2.o xZXZIP/extract3.o

XLOOK_DETECTOR_OBJS = xLook/ALASM/alasm.o xLook/BASIC/basic.o xLook/CC3/cc3.o \
                      xLook/DSQ/dsq.o xLook/HRUM/hrum.o xLook/HRUST1/hrust1.o \
                      xLook/HRUST21/hrust21.o xLook/HRUST22/hrust22.o xLook/LZS/lzs.o \
                      xLook/MASM/masm.o xLook/MSPACK/mspack.o xLook/PCD/pcd.o \
                      xLook/STORM/storm.o xLook/TASM/tasm.o xLook/TRUSH/trush.o \
                      xLook/XAS/xas.o xLook/ZXASM/zxasm.o \
                      xLook/XGFX/xgfx.o xLook/XGFX/formatFactory.o \
                      xLook/XGFX/formats/common.o xLook/XGFX/formats/screens.o \
                      xLook/XGFX/formats/gigascreen.o xLook/XGFX/formats/multicolor.o \
                      xLook/XGFX/formats/chunk.o xLook/XGFX/formats/bitplans.o \
                      xLook/XGFX/formats/8col.o xLook/XGFX/formats/fonts.o \
                      xLook/XGFX/formats/lc.o xLook/XGFX/formats/qhts.o \
                      xLook/XGFX/formats/sprites.o xLook/XGFX/formats/asc.o \
                      xLook/TRACKERS/trackers.o xLook/TRACKERS/module.o \
                      xLook/TRACKERS/pt2.o xLook/TRACKERS/pt3.o

$(XHRIP_OBJS) $(XZXZIP_OBJS) $(XLOOK_DETECTOR_OBJS) \
    xLook/xLook.o xLook/tools.o xLook/detector.o xLook/association.o \
    shared/hrust.o: CXXFLAGS += -MMD -MP

XLOOK_OBJS = shared/hrust.o xLook/xLook.o xLook/tools.o xLook/detector.o xLook/association.o \
             xLook/debug.o $(XLOOK_DETECTOR_OBJS)

XISD_OBJS = xISD/XiSD.o xISD/manager.o xISD/FmtReader.o xISD/tools.o xISD/iSDOS_tools.o \
            xISD/manager_tools.o xISD/manager_delete_files.o xISD/manager_get_files.o \
            xISD/manager_put_files.o xISD/manager_process_key.o \
            xISD/manager_make_folder.o xISD/iterator.o \
            xISD/Formats/IMG/img.o xISD/Formats/FDI/fdi.o xISD/Formats/FDI/filer.o 

.PHONY: all test-speccy release install clean zip zst deb version

all: $(TARGETS)

version:
	@echo $(VERSION)

test-speccy:
	python3 tests/speccy/run.py --sanitize $(if $(EXAMPLES),--examples "$(EXAMPLES)")

release: all
	mkdir -p $(RELEASE)
	(cd $(RELEASE) && mkdir -p $(TARGETS:%.far-plug-mb=%/plug))
	for d in $(TARGETS:%.far-plug-mb=%); do cp $$d/res/* $(RELEASE)/$$d/plug; mv $$d.far-plug-mb $(RELEASE)/$$d/plug; done

install: release
	sudo cp -r release/* $(TARGET_DIR)

# TODO: use template

xCreate.far-plug-mb: $(SHARED_BASIC_OBJS) $(XCREATE_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

xSCL.far-plug-mb: $(SHARED_OBJS) $(XSCL_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(XTRD_OBJS): xTRD/types.hpp

xTRD.far-plug-mb: $(SHARED_OBJS) $(XTRD_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

xISD.far-plug-mb: $(SHARED_OBJS) $(XISD_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

xHRiP.far-plug-mb: $(SHARED_OBJS) $(XHRIP_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

xZXZIP.far-plug-mb: $(SHARED_OBJS) $(XZXZIP_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

xLook.far-plug-mb: $(SHARED_BASIC_OBJS) $(XLOOK_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

-include $(XHRIP_OBJS:.o=.d) $(XZXZIP_OBJS:.o=.d) $(XLOOK_OBJS:.o=.d)

clean:
	rm $(SHARED_OBJS) $(XCREATE_OBJS) $(XSCL_OBJS) $(XTRD_OBJS) $(XISD_OBJS) $(XHRIP_OBJS) $(XZXZIP_OBJS) $(XLOOK_OBJS) $(TARGETS) || true
	rm -rf $(RELEASE) pkg src *.deb *.zip *.tar.zst *.pkg.tar.zst || true

# Installation archives

zip: far2l-plugins-speccy_$(VERSION)_amd64.zip

far2l-plugins-speccy_$(VERSION)_amd64.zip: release
	(cd $(RELEASE); zip -r $(PWD)/$@ *)

# Arch/Manjaro package (pacman -U …pkg.tar.zst). Keep pkgver in PKGBUILD in sync with VERSION.
ARCH_PKG = far2l-plugins-speccy-$(VERSION)-1-x86_64.pkg.tar.zst

zst: $(ARCH_PKG)

$(ARCH_PKG): release PKGBUILD
	sed -i 's/^pkgver=.*/pkgver=$(VERSION)/' PKGBUILD
	rm -rf pkg src
	makepkg -f
	test -f $@

# Debian deb package build

deb: far2l-plugins-speccy_$(VERSION)_amd64.deb

release/DEBIAN/control: DEBIAN-control.template release
	mkdir -p release/DEBIAN
	cp $< $@
	$(eval SIZE=$(shell sh -c "du -s release | grep -E -o '^[0-9]+'"))
	echo SIZE=$(SIZE)
	sed -i "s/<SIZE>/$(SIZE)/" $@
	sed -i "s/<VERSION>/$(VERSION)/" $@

far2l-plugins-speccy_$(VERSION)_amd64.deb: release/DEBIAN/control
	dpkg-deb --root-owner-group -b release $@