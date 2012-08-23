
.PHONY:	clean

usbdl:	usbdl.c usb_linux.c
	$(CC) -O2 -Wall -g -o usbdl usbdl.c usb_linux.c

clean:
	-rm usbdl
