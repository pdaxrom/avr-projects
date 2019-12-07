#include "usbconfig.h"
#include <avr/interrupt.h>
#include "therm_ds18b20.h"

# ifdef OTHERS_FUNCTIONS
	#include "usbdrv/usbdrv.h"
	
	void other_function1(void)
	{
//		usbPoll();
	}
#endif

volatile char THERM_DQ = 0;

void therm_select(unsigned char PIN_DS)
{
    THERM_DQ = PIN_DS;
}

uint8_t therm_reset(void)
{
    uint8_t i;

    //Pull line low and wait for 480uS
    ds_cbi(THERM_PORT, THERM_DQ);
    ds_sbi(THERM_DDR, THERM_DQ);
    _delay_us(480);
    //Release line and wait for 60uS
    ds_cbi(THERM_DDR,THERM_DQ);
    _delay_us(60);
    //Store line value and wait until the completion of 480uS period
    i = (THERM_PIN & _BV(THERM_DQ));
    _delay_us(420);
    //Return the value read from the presence pulse (0=OK, 1=WRONG)
    return i;
}

void therm_write_bit(uint8_t bit)
{
    //Pull line low for 1uS
    ds_cbi(THERM_PORT, THERM_DQ);
    ds_sbi(THERM_DDR, THERM_DQ);
    _delay_us(1);
    //If we want to write 1, release the line (if not will keep low)
    if (bit) {
	ds_cbi(THERM_DDR, THERM_DQ);
    }
    //Wait for 60uS and release the line
    _delay_us(60);
    ds_cbi(THERM_DDR, THERM_DQ);
}

uint8_t therm_read_bit(void)
{
    uint8_t bit=0;

    //Pull line low for 1uS
    ds_cbi(THERM_PORT, THERM_DQ);
    ds_sbi(THERM_DDR, THERM_DQ);
    _delay_us(1);
    //Release line and wait for 14uS
    ds_cbi(THERM_DDR, THERM_DQ);
    _delay_us(14);
    //Read line value
    if (THERM_PIN & _BV(THERM_DQ)) {
	bit=1;
    }
    //Wait for 45uS to end and return read value
    _delay_us(45);
    return bit;
}

uint8_t therm_read_byte(void)
{
    uint8_t i=8, n=0;

    while(i--) {
	//Shift one position right and store read value	
	n >>= 1;
	n |= (therm_read_bit() << 7);
	other_function1();
    }
    return n;
}

void therm_write_byte(uint8_t byte)
{
    uint8_t i=8;

    while(i--) {
	//Write actual bit and shift one position right to make the next bit ready
	therm_write_bit(byte&1);
	byte>>=1;
	other_function1();
    }
}

#if 0
void therm_read_temperature(unsigned char *buffer)
{
	//Reset, skip ROM and start temperature conversion
	int i = therm_reset();
	if (i == 0) {
	    PORTC ^= _BV(PINC1);
	}

	if ((THERM_PIN & _BV(THERM_DQ)) == 1) {
	    PORTC ^= _BV(PINC2);
	}

	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_CONVERTTEMP);
	//Wait until conversion is complete

        PORTC ^= _BV(PINC0);

	while (!therm_read_bit()) {
	    other_function1();
	}

        PORTC ^= _BV(PINC0);

	_delay_ms(800);
	//Reset, skip ROM and send command to read Scratchpad
	therm_reset();

	other_function1();

	therm_write_byte(THERM_CMD_SKIPROM);
	therm_write_byte(THERM_CMD_RSCRATCHPAD);
	//Read Scratchpad (only 2 first bytes)
	buffer[0]=therm_read_byte();
	other_function1();
	buffer[1]=therm_read_byte();
	other_function1();
	therm_reset();

	other_function1();
}
#endif
