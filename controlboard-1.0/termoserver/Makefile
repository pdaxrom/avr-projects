USBFLAGS = `libusb-config --cflags`
USBLIBS = `libusb-config --libs` #-framework CoreFoundation
EXE_SUFFIX =

CC	= gcc
CFLAGS	= $(USBFLAGS) -O -Wall -Ilib
LIBS	= $(USBLIBS) -lssl -lcrypto -lpthread

PROGRAM = tempserver$(EXE_SUFFIX)

all: $(PROGRAM)

$(PROGRAM): main.o lib/tcp.o
	$(CC) -o $@ $^ $(LIBS)

strip: $(PROGRAM)
	strip $(PROGRAM)

clean:
	rm -f *.o $(PROGRAM)
