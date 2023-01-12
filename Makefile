FAR_PATH ?= far2l

CXXFLAGS = -c -fPIC 
CXXFLAGS += -Ishared -I$(FAR_PATH)/WinPort -I$(FAR_PATH)/far2l/include -I$(FAR_PATH)/far2l/
CXXFLAGS += -DWINPORT_DIRECT -DWINPORT_REGISTRY -DUNICODE

LD=g++
LDFLAGS = -shared

TARGETS = xCreate.far-plug-mb xSCL.far-plug-mb xTRD.far-plug-mb
RELEASE = release

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

all: $(TARGETS)

release: all
	mkdir -p $(RELEASE)
	(cd $(RELEASE) && mkdir -p $(TARGETS:%.far-plug-mb=%/plug))
	for d in $(TARGETS:%.far-plug-mb=%); do cp $$d/res/* $(RELEASE)/$$d/plug; mv $$d.far-plug-mb $(RELEASE)/$$d/plug; done

# TODO: use template

xCreate.far-plug-mb: $(SHARED_BASIC_OBJS) $(XCREATE_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

xSCL.far-plug-mb: $(SHARED_OBJS) $(XSCL_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

xTRD.far-plug-mb: $(SHARED_OBJS) $(XTRD_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm $(SHARED_OBJS) $(XCREATE_OBJS) $(XSCL_OBJS) $(XTRD_OBJS) $(TARGETS) || true
	rm -rf $(RELEASE) || true
