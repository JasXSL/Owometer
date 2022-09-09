/*
	Prog settings:
	4mhz
	Startup time: 64ms
*/
#include <avr/sleep.h>
#include "Pitches.h"
const uint8_t PIN_BUZZER = PIN_PA1;
const uint8_t PIN_VELOSTAT_EN = PIN_PA6;
const uint8_t PIN_VELOSTAT_IN = PIN_PA2;
const uint32_t SLEEP_TIME = 30000;

uint32_t ON_TIME = 0;		// Time when it was turned on. 0 = off
uint8_t LAST_NOTE = 5;

const uint8_t NUM_LEDS = 5;
const uint8_t LEDS[NUM_LEDS] = {	// BGYOR
	PIN_PB1, PIN_PB0, PIN_PA3, PIN_PA5, PIN_PA4
};

const uint16_t NOTES[5] = {
	NOTE_C5,
	NOTE_D5,
	NOTE_E5,
	NOTE_F5
};

uint16_t lastFreq = 0;
uint16_t READING_THRESH = 0;	// Calibrated on boot

void setLEDs( uint8_t leds = 0 ){

	for( uint8_t i = 0; i < NUM_LEDS; ++i ){

		digitalWrite( LEDS[i], i < leds );

	}

}

uint8_t getStep( uint16_t reading ){

	uint16_t divider = 1000/90;				// Makes a divider for 70%
	uint16_t maxVal = (1024-READING_THRESH)*10/divider;	// Sets max to a value divider% between reading threshold and the max voltage
	// Prevent the reading from going below 0
	if( reading < READING_THRESH )
		reading = READING_THRESH;
	// Subtract READING_THRESH so reading is now between 0 and maxval
	reading = reading-READING_THRESH;
	// Map to a value between 0 and 1000
	const uint16_t percMax = 1000;
	uint32_t mapped = map(reading, 0,maxVal, 0,percMax);
	if( mapped > percMax-1 )	// minus 1 so it never goes to max.
		mapped = percMax-1;
	// Raise by 2
	//mapped = mapped*mapped/1000/2000+1;
	if( mapped < 300 )
		return 1;
	if( mapped < 500 )
		return 2;
	if( mapped < 800 )
		return 3;
	return 4;
}

void animStart(){
	// boot animation
	uint8_t pre = 0;
	for( uint8_t i = 0; i < NUM_LEDS*2; ++i ){

		uint8_t n = i;
		if( n >= NUM_LEDS )
			n = NUM_LEDS-1-(i-NUM_LEDS);
		
		digitalWrite(LEDS[pre], LOW);
		digitalWrite(LEDS[n], HIGH);
		delay(100);
		pre = n;

	}
}


ISR(RTC_PIT_vect){
  	RTC.PITINTFLAGS = RTC_PI_bm;          /* Clear interrupt flag by writing '1' (required) */
}


void setup(){

	pinMode(PIN_BUZZER, OUTPUT);
	pinMode(PIN_VELOSTAT_EN, OUTPUT);
	digitalWrite(PIN_VELOSTAT_EN, LOW);
	pinMode(PIN_VELOSTAT_IN, INPUT);

	/*
	Serial.begin(9600);
	delay(10);
	Serial.println("IT BEGINS");
	*/
	uint8_t i;
	for( i = 0; i < NUM_LEDS; ++i ){
		pinMode(LEDS[i], OUTPUT);
		digitalWrite(LEDS[i], LOW);
	}


	/* Initialize RTC: */
	while (RTC.STATUS > 0){
		;                                   /* Wait for all register to be synchronized */
	}
	RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;    /* 32.768kHz Internal Ultra-Low-Power Oscillator (OSCULP32K) */
	RTC.PITINTCTRL = RTC_PI_bm;           /* PIT Interrupt: enabled */
	RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc /* RTC Clock Cycles 32k, 1Hz ( */
	| RTC_PITEN_bm;                       /* Enable PIT counter: enabled */
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	
	
	

	// Calibrate
	digitalWrite(PIN_VELOSTAT_EN, HIGH);
	for( i = 0; i < 3; ++i ){
		
		delay(100);
		READING_THRESH += analogRead(PIN_VELOSTAT_IN);

	}
	READING_THRESH = READING_THRESH / 3 + 200;
	digitalWrite(PIN_VELOSTAT_EN, LOW);

	animStart();
	
	digitalWrite(LEDS[0], HIGH);

}

void loop(){

	// Do a reading
	digitalWrite(PIN_VELOSTAT_EN, HIGH);
	delay(1);
	const uint16_t reading = analogRead(PIN_VELOSTAT_IN);
	digitalWrite(PIN_VELOSTAT_EN, LOW);

	const uint32_t ms = millis();

	// Too low to turn anything on
	if( reading < READING_THRESH ){

		noTone(PIN_BUZZER);
		LAST_NOTE = 5;

		if( !ON_TIME || ms-ON_TIME > SLEEP_TIME )
			ON_TIME = 0;
		else
			setLEDs(1);
	}

	// High enough to turn on at least one	
	else{

		
		uint8_t note = getStep(reading);
		if( !ON_TIME && note > 3 ){
			
			animStart();
			ON_TIME = ms;	// Lets you continue
			note = 0;

		}

		// If off, we need an owometer 5 reading
		if( ON_TIME && note != LAST_NOTE ){
			
			ON_TIME = ms;		// Set it as on
			LAST_NOTE = note;
			if( note ){
				noTone(PIN_BUZZER);
				delay(5);
				tone(PIN_BUZZER, NOTES[note-1]);
			}
			else
				noTone(PIN_BUZZER);
			setLEDs(note+1);

		}
	}

	// It sleep
	if( !ON_TIME ){

		setLEDs(0);
		sleep_cpu();

	}
	else
		delay(250);

}

