#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>

#define LEDP	PORTC
#define LEDD	DDRC
#define LEDR	PINC0
#define LEDG	PINC1
#define LEDB	PINC2
#define LEDW	PINC3

int
main (void)
{
    //
//    DDRD  = 0b00000000; // Port D input
    PORTD = 0b11000000; // 6,7 pull-up resistors


//    DDRC = 0b00111000;

    LEDD |= _BV(LEDR) | _BV(LEDG) | _BV(LEDB) | _BV(LEDW);

    while(1) 
    {
        LEDP ^= _BV(LEDR);
        _delay_ms(800);
//if (PIND & _BV(PIND7)) {
        LEDP ^= _BV(LEDR);
        LEDP ^= _BV(LEDG);
        _delay_ms(800);
        LEDP ^= _BV(LEDG);
        LEDP ^= _BV(LEDB);
        _delay_ms(800);
        LEDP ^= _BV(LEDB);
//}
    }
}

