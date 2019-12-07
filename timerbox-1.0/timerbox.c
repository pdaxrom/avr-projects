#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>

#define LEDP	PORTC
#define LEDD	DDRC

#define LEDCHK	PINC4
#define LED5	PINC3
#define LED10	PINC2
#define LED15	PINC1
#define LED20	PINC0

#define BUTSET   PIND2
#define BUTSTART PIND3

#define BUTJITTER	3

#define INITCNTR	(255 - 32 + 1)

#if 1
static unsigned short delays[] = {
    5  * 60 * 8,
    10 * 60 * 8,
    15 * 60 * 8,
    20 * 60 * 8
};
#else
static unsigned short delays[] = {
    5  * 8,
    10 * 8,
    15 * 8,
    20 * 8
};
#endif

static unsigned short butSetJitter;
static unsigned char  delaySet;
static unsigned short delayCounter;

static void delayLed(char led, char on)
{
    led %= 4;
    if (on) {
	switch(led) {
	case 0: LEDP |= _BV(LED5);  break;
	case 1: LEDP |= _BV(LED10); break;
	case 2: LEDP |= _BV(LED15); break;
	case 3: LEDP |= _BV(LED20); break;
	}
    } else {
	switch(led) {
        case 0: LEDP &= ~_BV(LED5);  break;
        case 1: LEDP &= ~_BV(LED10); break;
        case 2: LEDP &= ~_BV(LED15); break;
        case 3: LEDP &= ~_BV(LED20); break;
	}
    }
}

ISR(TIMER2_OVF_vect)
{
    TCNT2 = INITCNTR;

    if (delayCounter > 0) {
	if ((delays[delaySet % 4] - delayCounter) % 8 < 4) {
	    LEDP |= _BV(LEDCHK);
	} else {
	    LEDP &= ~_BV(LEDCHK);
	}
	delayCounter--;
	if (delayCounter == 0) {
	    LEDP |= _BV(LEDCHK);
	}
	return;
    }

    PORTD &= ~_BV(PIND4);

    if (!(PIND & _BV(BUTSTART))) {
	delayCounter = delays[delaySet % 4];
	PORTD |= _BV(PIND4);
	return;
    }

    if (!(PIND & _BV(BUTSET)) && !butSetJitter) {
	LEDP &= ~_BV(LEDCHK);
	butSetJitter = BUTJITTER;
	delayLed(delaySet, 0);
	delaySet++;
	delayLed(delaySet, 1);
    }

    if (butSetJitter) {
	butSetJitter--;
    }
}

int main (void)
{
    butSetJitter = 0;
    delaySet = 0;
    delayCounter = 0;

    DDRD  = 0b00010000; // Pin D4 - relay
    PORTD = 0b00001100; // Pin D3,D2 pull-up resistors

    LEDD |= _BV(LED5) | _BV(LED10) | _BV(LED15) | _BV(LED20) | _BV(LEDCHK);
    delayLed(delaySet, 1);

    ASSR  = _BV(AS2);
    TCCR2 = _BV(CS22) | _BV(CS20);
    TCNT2 = INITCNTR;
    OCR2  = 0x00;
    TIMSK |= _BV(TOIE2); // Enable CTC interrupt

    sei(); // Enable global interrupts

    while(1);
}

