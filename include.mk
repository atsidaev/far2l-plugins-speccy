FAR_PATH ?= ../../far2l
CXXFLAGS = -g -c -fPIC -I../shared -I$(FAR_PATH)/WinPort -I$(FAR_PATH)/far2l/include -I$(FAR_PATH)/far2l/ -DWINPORT_DIRECT -DWINPORT_REGISTRY -DUNICODE

LD=g++
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
