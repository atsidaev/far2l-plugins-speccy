FAR_PATH ?= ../../far2l
CXXFLAGS = -c -fPIC -I../shared -I../../../far2l/WinPort -I../../../far2l/far2l/include -I../../../far2l/far2l/ -DWINPORT_DIRECT -DUNICODE

LD=g++ -g
LDFLAGS = -shared

ALLOBJ = $(OBJFILES)

all: $(TARGET)

$(TARGET) : $(OBJFILES)
	$(LD) $(LDFLAGS) $^ -o $@

#unload:
#	load.exe -u xTRD.dll
#load:
#	load.exe xTRD.dll

clean:
	rm $(TARGET) $(OBJFILES)
