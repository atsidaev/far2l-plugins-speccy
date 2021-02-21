.autodepend

TARGET   = xCreate.dll
OBJFILES = xCreate.obj registry.obj tools.obj creator.obj make_trd.obj make_fdi.obj make_scl.obj make_fdd.obj make_udi.obj make_td.obj

BCPATH = g:\

BCC = $(BCPATH)\bin\bcc32.exe
LINKER = $(BCPATH)\bin\tlink32.exe



CFLAGS =  -c -a1 -RT- -M- -x- -v- -I$(BCPATH)\include

LFLAGS = -L$(BCPATH)\lib -s -m -M -Tpd -aa -v-


ALLOBJ = $(OBJFILES)

ALLLIB = import32 cw32

#all :	unload	$(TARGET) load
#
#unload:
#	load.exe -u xZXZIP.dll
#load:
#	load.exe xZXZIP.dll

all : $(TARGET)
$(TARGET) : $(OBJFILES)
        $(LINKER) @&&|
 $(LFLAGS) +
$(ALLOBJ)
$(TARGET),
$(ALLLIB)
|

.cpp.obj:
        $(BCC) $(CFLAGS) $<
