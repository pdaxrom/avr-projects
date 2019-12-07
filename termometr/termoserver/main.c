/*
 * TCP server
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/select.h>
#endif
#include <pthread.h>
#include <usb.h>    /* this is libusb, see http://libusb.sourceforge.net/ */

#include "tcp.h"

#define USBDEV_SHARED_VENDOR    0x4242  /* VOTI */
#define USBDEV_SHARED_PRODUCT   0xe131  /* Obdev's free shared PID */

#define PSCMD_ECHO  0
#define PSCMD_GET   1
#define PSCMD_ON    2
#define PSCMD_OFF   3
#define PSCMD_TEMP  4

static int  usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
    char    buffer[256];
    int     rval, i;

    if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000)) < 0)
        return rval;
    if(buffer[1] != USB_DT_STRING)
        return 0;
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for(i=1;i<rval;i++){
        if(i > buflen)  /* destination buffer overflow */
            break;
        buf[i-1] = buffer[2 * i];
        if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }
    buf[i-1] = 0;
    return i-1;
}

#define USB_ERROR_NOTFOUND  1
#define USB_ERROR_ACCESS    2
#define USB_ERROR_IO        3

static int usbOpenDevice(usb_dev_handle **device, int vendor, char *vendorName, int product, char *productName)
{
    struct usb_bus      *bus;
    struct usb_device   *dev;
    usb_dev_handle      *handle = NULL;
    int                 errorCode = USB_ERROR_NOTFOUND;
    static int          didUsbInit = 0;

    if(!didUsbInit){
        didUsbInit = 1;
        usb_init();
    }
    usb_find_busses();
    usb_find_devices();
    for(bus=usb_get_busses(); bus; bus=bus->next){
        for(dev=bus->devices; dev; dev=dev->next){
            if(dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product){
                char    string[256];
                int     len;
                handle = usb_open(dev); /* we need to open the device in order to query strings */
                if(!handle){
                    errorCode = USB_ERROR_ACCESS;
                    fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
                    continue;
                }
                if(vendorName == NULL && productName == NULL){  /* name does not matter */
                    break;
                }
                /* now check whether the names match: */
                len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
                if(len < 0){
                    errorCode = USB_ERROR_IO;
                    fprintf(stderr, "Warning: cannot query manufacturer for device: %s\n", usb_strerror());
                }else{
                    errorCode = USB_ERROR_NOTFOUND;
                    /* fprintf(stderr, "seen device from vendor ->%s<-\n", string); */
                    if(strcmp(string, vendorName) == 0){
                        len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
                        if(len < 0){
                            errorCode = USB_ERROR_IO;
                            fprintf(stderr, "Warning: cannot query product for device: %s\n", usb_strerror());
                        }else{
                            errorCode = USB_ERROR_NOTFOUND;
                            /* fprintf(stderr, "seen product ->%s<-\n", string); */
                            if(strcmp(string, productName) == 0)
                                break;
                        }
                    }
                }
                usb_close(handle);
                handle = NULL;
            }
        }
        if(handle)
            break;
    }
    if(handle != NULL){
        errorCode = 0;
        *device = handle;
    }
    return errorCode;
}

static void thread_client(void *arg) {
    usb_dev_handle	*handle = NULL;
    unsigned char	buffer[8];
    int			nBytes;
    char		str[256];
    int			r;

    tcp_channel *client = (tcp_channel *) arg;

    if(usbOpenDevice(&handle, USBDEV_SHARED_VENDOR, "dev.ukrinteltech.com", USBDEV_SHARED_PRODUCT, "Control board") != 0) {
        fprintf(stderr, "Could not find USB device \"Control board\" with vid=0x%x pid=0x%x\n", USBDEV_SHARED_VENDOR, USBDEV_SHARED_PRODUCT);
	tcp_close(client);
        return;
    }

    nBytes = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, PSCMD_TEMP, 0, 0, (char *)buffer, sizeof(buffer), 5000);
    if(nBytes < 2) {
	if(nBytes < 0) {
	    fprintf(stderr, "USB error: %s\n", usb_strerror());
	}
	fprintf(stderr, "only %d bytes status received\n", nBytes);
	tcp_close(client);
	return;
    }

    int digit = buffer[0] >> 4;
    digit |= (buffer[1] & 0x07) << 4;

    int decimal = buffer[0] & 0xf;
    decimal *= 5;

    if (buffer[1] > 0xFB) {
	digit = -(127 - digit);
    }

    snprintf(str, sizeof(str), "%.02f\n", ((float)digit + (float)decimal / 100));

    if ((r = tcp_write(client, str, strlen(str) + 1)) <= 0) {
	fprintf(stderr, "tcp_write()\n");
    }

    usb_close(handle);

    tcp_close(client);
}

int main(int argc, char *argv[])
{
    usb_init();

    tcp_channel *server = tcp_open(TCP_SERVER, NULL, 8082);
    if (!server) {
	fprintf(stderr, "tcp_open()\n");
	return -1;
    }

    while(1) {
	pthread_t tid;
	int r;

	tcp_channel *client = tcp_accept(server);
	if (!client) {
	    fprintf(stderr, "tcp_accept()\n");
	    return -1;
	}

	r = pthread_create(&tid, NULL, (void *) &thread_client, (void *) client);
	if(r) {
	    fprintf(stderr, "pthread_create(thread_client)\n");
	    tcp_close(client);
	    break;
	}
    }

    tcp_close(server);

    return 0;
}
