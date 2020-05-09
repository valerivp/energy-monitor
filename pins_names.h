#ifndef PINS_NAMES_H
#define PINS_NAMES_H


#pragma once
//#undef Pins_Arduino_h
//#include "pins_arduino.h"



#if defined(ARDUINO_ESP8266_ESP01)

enum pins : uint8_t {
	D3 = 0,
	D4 = 2,
	GPIO0 = 0,
	GPIO2 = 2,
	RX = 3,
	TX = 1,
	LED = 3
};
#elif defined(ARDUINO_ESP8266_GENERIC) && defined(Sonoff)

enum pins : uint8_t {
	gpio14 = 14,
	rx = 3,
	tx = 1,
	led = 13,
	button = 0,
	relay = 12
};
#elif defined(ARDUINO_ESP8266_NODEMCU) || defined(ARDUINO_ESP8266_WEMOS_D1MINI)

enum pins : uint8_t {
	d0 = 16,
	d1 = 5, scl = 20,
	d2 = 4, sda = 20,
	d3 = 0,
	d4 = 2, led = 2,
	d5 = 14,
	d6 = 12,
	d7 = 13,
	d8 = 15,
	rx = 3,
	tx = 1,
	button = 0,

};
#elif defined(ESP32)
enum pins : uint8_t {
	tx = 1, //TX,
	rx = 3, //RX,
	sda = 23, //SDA,
	scl = 19, //SCL,
	led = 22 //LED_BUILTIN

};
#elif defined(ARDUINO_AVR_MEGA2560)
//#include "../generic/common.h"
enum pins : uint8_t {
	tx = 0,
	rx = 1,
	led = 13,
	d16 = 16, tx2 = 16,
	d17 = 17, rx2 = 17,
	d18 = 18, tx1 = 18,
	d19 = 19, rx1 = 19,
	d20 = 20, sda = 20,
	d21 = 21, scl = 21,
	d23 = 23,
	d24 = 24,
	d25 = 25,
	d26 = 26,
	d27 = 27,
	d28 = 28,
	d29 = 29,
	d30 = 30,
	d31 = 31,
	d32 = 32,
	d33 = 33,
	d34 = 34,
	d35 = 35,
	d36 = 36,
	d37 = 37,
	d38 = 38,
	d39 = 39,
	d40 = 40,
	d41 = 41,
	d42 = 42,
	d43 = 43,
	d44 = 44,
	d45 = 45,
	d46 = 46,
	d47 = 47,
	d48 = 48,
	d49 = 49,
	d50 = 50, spi_miso = 50,
	d51 = 51, spi_mosi = 51,
	d52 = 52, spi_sck = 52,
	d53 = 53, spi_ss = 53,
	a0 = 54,
	a1 = 55,
	a2 = 56,
	a3 = 57,
	a4 = 58,
	a5 = 59,
	a6 = 60,
	a7 = 61,
	a8 = 62,
	a9 = 63,
	a10 = 64,
	a11 = 65,
	a12 = 66,
	a13 = 67,
	a14 = 68,
	a15 = 69
};
#elif defined(ARDUINO_AVR_NANO)

enum pins : uint8_t {
	led = 13
};

#endif

#endif