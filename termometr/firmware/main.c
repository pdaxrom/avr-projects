/* Name: eyespyio.c
 * Project: EyeSpy IO board
 * Author: http://dev.tusur.info
 * Creation Date: 2010-06-25
 * Tabsize: 4
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 */

#include "usbconfig.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"

#include "therm_ds18b20.h"

/* ----------------------- hardware I/O abstraction ------------------------ */

unsigned char tempbuf[2] = {0x0, 0x0};
int tempStage;

/* pin assignments:
PC4	Key 1
PC5	Key 2

PD0	USB-
PD1	debug tx
PD2	USB+ (int0)
PD3	Key 13
PD4	Key 14
PD5	Key 15
PD6	Key 16
PD7	Key 17
*/

static void hardwareInit(void)
{
    uchar	i, j;

    DDRB = 0b00000000;
    PORTB = 0b00000000;
    PORTC = 0x30;   /* pull-up PC4, PC5 */
    DDRC = _BV (PC0) | _BV(PC1) | _BV(PC2) | _BV(PC3); /* PC0, PC1, PC2, PC3 are digital output */
    PORTC |= 0x0f; /* LEDS off */

    PORTD = 0xfa;   /* 1111 1010 bin: activate pull-ups except on USB lines */
    DDRD = 0x07;    /* 0000 0111 bin: all pins input except USB (-> USB reset) */
	j = 0;
	while(--j){     /* USB Reset by device only required on Watchdog Reset */
		i = 0;
		while(--i); /* delay >10ms for USB reset */
	}
    DDRD = 0x02;    /* 0000 0010 bin: remove USB reset condition */
    /* configure timer 0 for a rate of 12M/(1024 * 256) = 45.78 Hz (~22ms) */
    TCCR0 = 5;      /* timer 0 prescaler: 1024 */
}

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

uchar	usbFunctionSetup(uchar data[8])
{
    usbRequest_t    *rq = (void *)data;
    static uchar    replyBuf[2];

    usbMsgPtr = replyBuf;
    if(rq->bRequest == 0){  /* ECHO */
        replyBuf[0] = rq->wValue.bytes[0];
        replyBuf[1] = rq->wValue.bytes[1];
        return 2;
    }

    if(rq->bRequest == 1){  /* GET_STATUS -> result = 2 bytes */
        replyBuf[0] = ~(PINC >> 4);
        replyBuf[1] = 0;
        return 2;
    }

    if(rq->bRequest == 2){  /* ON */
	if (rq->wValue.bytes[0] < 4)
	    PORTC &= ~(1 << rq->wValue.bytes[0]);
    }

    if(rq->bRequest == 3){  /* OFF */
	if (rq->wValue.bytes[0] < 4)
    	    PORTC |= (1 << rq->wValue.bytes[0]);
    }

    if(rq->bRequest == 4){  /* ECHO */
        replyBuf[0] = tempbuf[0];
        replyBuf[1] = tempbuf[1];
        return 2;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

void readTemp()
{
    if (tempStage == 0) {
	//Reset, skip ROM and start temperature conversion
	if (therm_reset() == 0) {
	    tempStage = 1;
	}
    } else if (tempStage == 1) {
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERTTEMP);
	tempStage = 2;
    } else if (tempStage == 2) {
	if (therm_read_bit()) {
	    tempStage = 3;
	    int i;
	    for (i = 0; i < 25; i++) {
		_delay_us(450);
		usbPoll();
	    }
	}
    } else if (tempStage == 3) {
	if (therm_reset() == 0) {
	    tempStage = 4;
	}
    } else if (tempStage == 4) {
	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_RSCRATCHPAD);
	tempStage = 5;
    } else if (tempStage == 5) {
	unsigned char a = therm_read_byte();
	unsigned char b = therm_read_byte();
	tempbuf[0] = a;
	tempbuf[1] = b;
	tempStage = 6;
    } else if (tempStage == 6) {
	if (therm_reset() == 0) {
	    tempStage = 0;
	}
    }
}

int	main(void)
{
    tempStage = 0;

    hardwareInit();
    odDebugInit();
    usbInit();
    sei();
    DBG1(0x00, 0, 0);
    therm_select(PB0);

    for(;;){
	usbPoll();
	readTemp();
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
