#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#pragma once

#include <Arduino.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <time.h>
#ifdef __cplusplus
}
#endif


#include "flashstr.h"

#include <Streaming.h>
#include "debug.h"

#include "pins_names.h"


#include "config_def.h"




#define sizeofArray(array) (sizeof(array) / sizeof(array[0]))

const char * toHex(uint8_t x);
const char * toHex(uint16_t x);
const char * toBin(uint8_t x);

const char * strrchar(char* ptr, char c);

uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc = 0);
uint8_t crc8(const uint8_t *addr, uint8_t len);

#if 0//ndef ESP8266
template< typename T > int EEPROM_read(int idx, T &t) {
	uint8_t *ptr = (uint8_t*)&t;
	for (int i = 0; i < sizeof(T); i++) {
		ptr[i] = eeprom_read_byte(idx);
		idx++;
		
	};
	return idx;
}
template< typename T > int EEPROM_update(int idx, const T &t) {
	const uint8_t *ptr = (const uint8_t*)&t;
	for (int i = 0; i < sizeof(T); i++) {
		if (eeprom_read_byte(idx) != ptr[i])
			eeprom_write_byte(idx, ptr[i]);
		idx++;
	};
	return idx;
}
//uint16_t EEPROM_crc16(int idx, int len, uint16_t crc);

template< typename T > T EEPROM_crc16(int idx, int len, T crc) {
	for (int i = 0; i < len; i++) {
		uint8_t b = eeprom_read_byte(idx);
		crc = crc16(&b, 1, crc);
		idx++;
	};
	return crc;
}

#endif

template <typename T>
bool inrange(T arg, T lv, T rv, const char* Desc) {
	if ((lv <= arg) && (arg <= rv))
		return true;
	if (Desc)
		DEBUG_PRINT(Desc << (" ") << arg << (" not in range ") << lv << ("...") << rv);
	return false;
};
template <typename T>
bool inrange(T arg, T lv, T rv) {
	return ((lv <= arg) && (arg <= rv));
};

// format time YYYYmmDDTHHMMSS
const char * time2str(time_t t, const char * format = NULL);
// time from string YYYYmmDDTHHMMSS
time_t str2time(const char* str);


#endif // !COMMON_DEF_H
