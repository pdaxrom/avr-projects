#define WARNING_TIME	15
#define WARNING_SLEEP_TIME	30
#define ALARM_TIME	15
#define WARNING_COLLECTOR_THRESHOLD	20

#define F_CPU 12000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>

#define FPCK	1024
#define TICK1HZ		(F_CPU / FPCK / 1)
#define TICK10HZ	(F_CPU / FPCK / 10)

#define BLINK_INFO	40
#define BLINK_WARN	20
#define BLINK_ALARM	5

#define LEDP	PORTC
#define LEDD	DDRC
#define LEDR	PINC3
#define LEDG	PINC4
#define LEDB	PINC5

#define LEDON(l)	LEDP |= _BV(l);
#define LEDOFF(l)	LEDP &= ~_BV(l);

#define RELAYON()	PORTB |= _BV(PINB0);
#define RELAYOFF()	PORTB &= ~_BV(PINB0);

#define INPP	PIND
#define INTPOWR	PIND5
#define INTSOFT	PIND6
#define INTHARD	PIND7

struct Input {
    unsigned char shock_soft;
    unsigned char shock_hard;
    unsigned char ignition;
};

static volatile struct Input input;

static volatile int led_type;

static volatile int tick_counter;
static volatile int warning_counter;
static volatile int alarm_counter;

static volatile int warning_sleep_counter;
static volatile int warning_collector;

enum {
    NONE = 0,
    INFO,
    WARN,
    ALARM
};

static const int blink_delay[] = {
    0,
    BLINK_INFO,
    BLINK_WARN,
    BLINK_ALARM
};

static void led_on(char type)
{
    switch(type) {
    case INFO:  LEDON(LEDB); break;
    case WARN:  LEDON(LEDG); break;
    case ALARM: LEDON(LEDR); break;
    }
}

static void led_off(char type)
{
    switch(type) {
    case INFO:  LEDOFF(LEDB); break;
    case WARN:  LEDOFF(LEDG); break;
    case ALARM: LEDOFF(LEDR); break;
    }
}

ISR(TIMER1_COMPA_vect)
{
    int force_alarm = 0;

    if (tick_counter < 1) {
	led_on(led_type);
    } else {
	led_off(led_type);
    }

    tick_counter++;

    if (warning_counter) {
	if (!--warning_counter) {
	    led_off(led_type);
	    led_type = INFO;
	    tick_counter = 0;
	}
    }

    if (warning_sleep_counter) {
	if (input.shock_soft) {
	    warning_collector++;
	    if (warning_collector > WARNING_COLLECTOR_THRESHOLD * 10) {
		force_alarm = 1;
	    }
	}
	warning_sleep_counter--;
    }

    if (input.shock_soft && !warning_counter && !alarm_counter) {
	led_off(led_type);
	led_type = WARN;
	tick_counter = 0;
	warning_counter = WARNING_TIME * 10;
	if (!warning_sleep_counter) {
	    warning_collector = 0;
	}
	warning_sleep_counter = WARNING_SLEEP_TIME * 10 * 3;
    }

    if (alarm_counter) {
	if (!--alarm_counter) {
	    led_off(led_type);
	    led_type = INFO;
	    tick_counter = 0;

	    RELAYOFF();
	}
    }

    if ((input.shock_hard && !alarm_counter) || force_alarm) {
	led_off(led_type);
	led_type = ALARM;
	tick_counter = 0;
	warning_counter = 0;
	warning_sleep_counter = 0;
	alarm_counter = ALARM_TIME * 10;

	RELAYON();
    }

    if (tick_counter >= blink_delay[led_type]) {
	tick_counter = 0;
    }
}

int main (void)
{
    led_type = INFO;
    tick_counter = 0;
    warning_counter = 0;
    alarm_counter = 0;
    warning_sleep_counter = 0;
    warning_collector = 0;

    LEDD |= _BV(LEDR) | _BV(LEDG) | _BV(LEDB);

    DDRB |= _BV(PINB0); // Port B pin 0 out

//    DDRD  = 0b00000000; // Port D input
    PORTD = 0b11000000; // 6,7 pull-up resistors

    TCCR1B |= (1 << WGM12); // Configure timer 1 for CTC mode 
    TIMSK |= (1 << OCIE1A); // Enable CTC interrupt
    sei(); // Enable global interrupts
    TCCR1B |= _BV(CS12) | _BV(CS10); // Fcpu / 1024
    OCR1A = TICK10HZ;

    while(1)
    {
	char tmp = INPP & _BV(INTPOWR);
	if (tmp != input.ignition) {
	    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		input.ignition = tmp;
	    }
	}

	tmp = !(INPP & _BV(INTSOFT));
	if (tmp != input.shock_soft) {
	    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		input.shock_soft = tmp;
	    }
	}

	tmp = !(INPP & _BV(INTHARD));
	if (tmp != input.shock_hard) {
	    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		input.shock_hard = tmp;
	    }
	}

    }
}

static volatile const char copyright[] = "(c) UkrIntelTech, 2013";
