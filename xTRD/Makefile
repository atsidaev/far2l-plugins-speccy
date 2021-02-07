.autodepend

TARGET   = xTRD.dll
OBJFILES = debug.obj xTRD.obj manager.obj memory.obj fmtreader.obj tools.obj mngr_tools.obj mngr_dir.obj mngr_del.obj mngr_get.obj mngr_put.obj registry.obj detector.obj mngr_key.obj

BCPATH = g:\

BCC = $(BCPATH)\bin\bcc32.exe
LINKER = D:\Language\BC5\bin\tlink32.exe



CFLAGS =  -c -a1 -RT- -M- -x- -v- -I$(BCPATH)\include

LFLAGS = -L$(BCPATH)\lib -Tpd -aa -v-


ALLOBJ = $(OBJFILES)

ALLLIB = import32.lib cw32.lib

all :		$(TARGET) 

#unload:
#	load.exe -u xTRD.dll
#load:
#	load.exe xTRD.dll

$(TARGET) : $(OBJFILES)
        $(LINKER) @&&|
 $(LFLAGS) +
$(ALLOBJ)
$(TARGET),
$(ALLLIB)
|

.cpp.obj:
        $(BCC) $(CFLAGS) $<
