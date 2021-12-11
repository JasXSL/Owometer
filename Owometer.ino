/*
	Prog settings:
	4mhz
	Startup time: 64ms
*/
#include <avr/sleep.h>
#include "Pitches.h"
const uint8_t PIN_BUZZER = 0;
const uint8_t PIN_VELOSTAT_EN = 2;
const uint8_t PIN_VELOSTAT_IN = 1;
const uint32_t SLEEP_TIME = 30000;

uint32_t ON_TIME = 0;		// Time when it was turned on. 0 = off
uint8_t LAST_NOTE = 5;

const uint8_t NUM_LEDS = 5;
const uint8_t LEDS[NUM_LEDS] = {
	6, 7, 8, 9, 10
};

const uint16_t NOTES[5] = {
	NOTE_A4,
	NOTE_AS4,
	NOTE_B4,
	NOTE_C5
};

uint16_t lastFreq = 0;
uint16_t READING_THRESH = 700;

void setLEDs( uint8_t leds = 0 ){

	for( uint8_t i = 0; i < NUM_LEDS; ++i ){

		digitalWrite( LEDS[i], i < leds );

	}

}

uint8_t getStep( uint16_t reading ){

	reading = min(reading, 1000);
	reading -= READING_THRESH;
	uint16_t max = 1000-READING_THRESH;
	
	float perc = pow((float)reading/max, 4);
	return max((uint8_t)(perc*100)/25, 1);

}

void setTone( uint16_t freq ){

	noTone(PIN_BUZZER);
	delay(5);
	tone(PIN_BUZZER, freq);

}

void animStart(){
	
	uint8_t pre = 0;
	for(uint8_t i = 0; i < NUM_LEDS*2; ++i ){

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
void sleep(){
	sleep_cpu();
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
	for( uint8_t i = 0; i < NUM_LEDS; ++i ){
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
	
	
	animStart();


	digitalWrite(PIN_VELOSTAT_EN, HIGH);
	delay(100);
	READING_THRESH = analogRead(PIN_VELOSTAT_IN)+100;
	digitalWrite(PIN_VELOSTAT_EN, LOW);
	
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
			if( note )
				setTone(NOTES[note-1]);
			else
				noTone(PIN_BUZZER);
			setLEDs(note+1);

		}
	}

	// It sleep
	if( !ON_TIME ){

		setLEDs(0);
		sleep();

	}
	else
		delay(250);

}

