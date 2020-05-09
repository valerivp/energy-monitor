#ifndef CONFIG_DEF_H
#define CONFIG_DEF_H

#pragma once

#ifdef UltraWebServer
#define HOST_NAME "ThermoController"
#define USE_WEBSOCKETS
#define USE_FILES_EDIT
#define USE_FILES_READ
#define USE_DNS
//#define USE_BITLASH
//#define USE_NTP_SERVER
//#define USE_NTP_CLIENT
#define USE_RTC_CLOCK
#define USE_433_RECEIVER
#ifdef ESP8266
	#define USE_FTP_SERVER
#endif // ESP8266


#define AP_NAME_PREFIX "TC-"

#elif defined(ItsSensor)
#define HOST_NAME "UWSSensor"
//#define USE_NTP_CLIENT
//#define AP_NAME_PREFIX "uwss_ESP"
//#define USE_FILES_READ

//#define USE_DNS

#define USE_433_RECEIVER
//#define USE_1WIRE_SENSORS
//#define USE_SENSORS_ARRAY
//#define USE_NTP_CLIENT

#elif defined(Sonoff)
#define HOST_NAME "Sonoff"
//#define USE_NTP_CLIENT
//#define AP_NAME_PREFIX "uwss_ESP"
//#define USE_FILES_READ

//#define USE_DNS

//#define USE_433_RECEIVER
//#define USE_1WIRE_SENSORS
//#define USE_SENSORS_ARRAY
//#define USE_NTP_CLIENT

#define AP_NAME_PREFIX "Sonoff-"

#elif defined(EnergyMonitor)
#define HOST_NAME "EnergyMonitor"
#define AP_NAME_PREFIX "EM-"
#define USE_FILES_READ
#define USE_FILES_EDIT

#define USE_NTP_CLIENT
#define USE_WEBSOCKETS

//#define USE_RTC_CLOCK

#endif 

#ifdef USE_433_RECEIVER
	#if defined(ARDUINO_ESP8266_ESP01)
		#define RECEIVER_433_DATA_PIN pins::RX
	#elif defined(ARDUINO_ESP8266_NODEMCU) || defined(ARDUINO_ESP8266_WEMOS_D1MINI)
		#define RECEIVER_433_DATA_PIN pins::d5
	#elif defined(ESP32)
		#define RECEIVER_433_DATA_PIN 25
	#endif
#endif

#ifdef USE_1WIRE_SENSORS
	#if defined(ARDUINO_ESP8266_ESP01)
		#define RECEIVER_1WIRE_DATA_PIN pins::GPIO2
	#elif defined(ARDUINO_ESP8266_NODEMCU)
		#define RECEIVER_1WIRE_DATA_PIN pins::D2
	#endif
#endif


#endif // !CONFIG_DEF_H
