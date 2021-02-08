FAR_PATH ?= ../../far2l
CXXFLAGS = -c -fPIC -I../shared -I$(FAR_PATH)/WinPort -I$(FAR_PATH)/far2l/include -I$(FAR_PATH)/far2l/ -DWINPORT_DIRECT -DUNICODE

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
