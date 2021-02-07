.autodepend

TARGET   = xSCL.dll
OBJFILES = debug.obj xSCL.obj memory.obj tools.obj manager.obj manager_put.obj manager_get.obj manager_del.obj manager_key.obj manager_tools.obj registry.obj detector.obj

BCPATH = g:\

BCC = $(BCPATH)\bin\bcc32.exe
LINKER = $(BCPATH)\bin\tlink32.exe



CFLAGS =  -c -a1 -RT- -M- -x- -v- -I$(BCPATH)\include

LFLAGS = -L$(BCPATH)\lib -s -m -M -Tpd -aa -v-


ALLOBJ = $(OBJFILES)

ALLLIB = import32 cw32

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
