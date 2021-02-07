.autodepend

TARGET   = fdd.fmt
OBJFILES = fdd.obj filer.obj memory.obj

BCPATH = g:\

BCC = $(BCPATH)\bin\bcc32.exe
LINKER = $(BCPATH)\bin\tlink32.exe



CFLAGS =  -c -a1 -RT- -M- -x- -v- -I$(BCPATH)\include

LFLAGS = -L$(BCPATH)\lib -s -m -M -Tpd -aa -v-


ALLOBJ = $(OBJFILES)

ALLLIB = import32.lib cw32.lib

all :	$(TARGET)

$(TARGET) : $(OBJFILES)
        $(LINKER) @&&|
 $(LFLAGS) +
$(ALLOBJ)
$(TARGET),
$(ALLLIB)
|

.cpp.obj:
        $(BCC) $(CFLAGS) $<
