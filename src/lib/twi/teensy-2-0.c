/* ----------------------------------------------------------------------------
 * Very simple Teensy 2.0 TWI library : code
 *
 * - This is mostly straight from the datasheet, section 20.6.6, figure 20-11
 *   (the code example in C), and section 20.8.1, figure 20-12
 * - Also see the documentation for `<util/twi.h>` at
 *   <http://www.nongnu.org/avr-libc/user-manual/group__util__twi.html#ga8d3aca0acc182f459a51797321728168>
 *
 * Some other (more complete) TWI libraries for the Teensy 2.0 (and other Atmel
 * processors):
 * - [i2cmaster] (http://homepage.hispeed.ch/peterfleury/i2cmaster.zip)
 *   - written by [peter-fleury] (http://homepage.hispeed.ch/peterfleury/)
 * - [the arduino twi library]
 *   (https://github.com/arduino/Arduino/tree/master/libraries/Wire/utility)
 *   - look for an older version if you need one that doesn't depend on all the
 *     other Arduino stuff
 * ----------------------------------------------------------------------------
 * Copyright (c) 2012 Ben Blazak <benblazak.dev@gmail.com>
 * Released under The MIT License (MIT) (see "license.md")
 * Project located at <https://github.com/benblazak/ergodox-firmware>
 * ------------------------------------------------------------------------- */


// ----------------------------------------------------------------------------
// conditional compile
#if MAKEFILE_BOARD == teensy-2-0
// ----------------------------------------------------------------------------

#include <stdbool.h>
#include <util/delay.h>
#include <util/twi.h>
#include "./teensy-2-0.h"

// ----------------------------------------------------------------------------

static bool twi_fault_detected = false;

static bool twi_fault() {
    // There's a pull-up on each I2C line, so test for problems by driving each
    // line low and watching the other one. If it goes low too, they're stuck
    // together (as happens when your keyboard was built with a TRRS jack with
    // integrated switch such as the SJ-43515TS rather than SJ-43514 and
    // there's nothing plugged into it).
    //
    // If they are stuck, set the fault flag and don't do anything even if
    // asked to later because the hardware gets wedged if that's the case.
    bool failing = true;
    uint8_t ddrd_orig = DDRD;
    PORTD = 0;
    
    // Emit stuff on SCL (PD0)
    DDRD = 0x01;
    _delay_us(1);
    failing &= (PIND & 2) == 0;

    // Emit stuff on SDA (PD1)
    DDRD = 0x02;
    _delay_us(1);
    failing &= (PIND & 1) == 0;

    // Restore I/O control state
    DDRD = ddrd_orig;

    twi_fault_detected = failing;
    return failing;
}

void twi_init(void) {
    if (twi_fault())
        return;

	// set the prescaler value to 0
	TWSR &= ~( (1<<TWPS1)|(1<<TWPS0) );
	// set the bit rate
	// - TWBR should be 10 or higher (datasheet section 20.5.2)
	// - TWI_FREQ should be 400000 (400kHz) max (datasheet section 20.1)
	TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
}

uint8_t twi_start(void) {
    if (twi_fault_detected)
        return -1;

	// send start
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA);
	// wait for transmission to complete
	while (!(TWCR & (1<<TWINT)));
	// if it didn't work, return the status code (else return 0)
	if ( (TW_STATUS != TW_START) &&
	     (TW_STATUS != TW_REP_START) )
		return TW_STATUS;  // error
	return 0;  // success
}

void twi_stop(void) {
    if (twi_fault_detected)
        return;

	// send stop
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	// wait for transmission to complete
	while (TWCR & (1<<TWSTO));
}

uint8_t twi_send(uint8_t data) {
    if (twi_fault_detected)
        return -1;

	// load data into the data register
	TWDR = data;
	// send data
	TWCR = (1<<TWINT)|(1<<TWEN);
	// wait for transmission to complete
	while (!(TWCR & (1<<TWINT)));
	// if it didn't work, return the status code (else return 0)
	if ( (TW_STATUS != TW_MT_SLA_ACK)  &&
	     (TW_STATUS != TW_MT_DATA_ACK) &&
	     (TW_STATUS != TW_MR_SLA_ACK) )
		return TW_STATUS;  // error
	return 0;  // success
}

uint8_t twi_read(uint8_t * data) {
    if (twi_fault_detected)
        return -1;

	// read 1 byte to TWDR, send ACK
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	// wait for transmission to complete
	while (!(TWCR & (1<<TWINT)));
	// set data variable
	*data = TWDR;
	// if it didn't work, return the status code (else return 0)
	if (TW_STATUS != TW_MR_DATA_ACK)
		return TW_STATUS;  // error
	return 0;  // success
}


// ----------------------------------------------------------------------------
#endif
// ----------------------------------------------------------------------------

