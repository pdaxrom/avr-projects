#define F_CPU 1000000UL

#include <avr/io.h>
//#include <avr/iom8.h>
#include <avr/interrupt.h>
//#include <util/delay.h>

#define LEDP	PORTC
#define LEDD	DDRC
#define LEDR	PINC3
#define LEDG	PINC4
#define LEDB	PINC5

#define FCLK	12000000
#define FPCK	1024

#define TICK1HZ		(FCLK / FPCK / 1)
#define TIMVAL1HZ	(65535 - TICK1HZ)

#define TICK10HZ	(FCLK / FPCK / 10)
#define TIMVAL10HZ	(65535 - TICK10HZ)

int tick_count;

ISR(TIMER1_COMPA_vect)
{
    if (tick_count < 2) {
	LEDP ^= _BV(LEDB);
    }

    tick_count++;

    if (tick_count > 20) {
	tick_count = 0;
    }
//    TCNT1 = TIMVAL10HZ;
}

int main (void)
{
    //
//    DDRD  = 0b00000000; // Port D input
    PORTD = 0b11000000; // 6,7 pull-up resistors

    LEDD |= _BV(LEDR) | _BV(LEDG) | _BV(LEDB);

    tick_count = 0;

    TCCR1B |= (1 << WGM12); // Configure timer 1 for CTC mode 
    TIMSK |= (1 << OCIE1A); // Enable CTC interrupt

    sei(); // Enable global interrupts

    TCCR1B |= _BV(CS12) | _BV(CS10); // Fcpu / 1024

    OCR1A = TICK10HZ;

    while(1)
    {
    }
}
