#include "common_def.h"

#if defined(__AVR__)
#include <util/crc16.h>
#endif



// format time YYYYmmDDTHHMMSS
const char * time2str(time_t t, const char * format) {
	static char TimeStringBuf[16]; //20140527T000000

	tm* timestruct = gmtime(&t);
//	strftime(TimeStringBuf, sizeof(TimeStringBuf), (format ? format : "%Y%m%dT%H%M%S"), timestruct);
	return TimeStringBuf;
}


// time from string YYYYmmDDTHHMMSS
time_t str2time(const char* str) {
	if (strlen(str) != 15 || str[8] != 'T')
		return 0;
	char * end;
	unsigned long d = strtoul(str, &end, 10);
	if (end != &str[8])
		return 0;
	unsigned long t = strtoul(&str[9], &end, 10);
	if (end != &str[15])
		return 0;

	tm dt;
	dt.tm_mday = d % 100; if (!inrange((int8_t)dt.tm_mday, (int8_t)1, (int8_t)31)) return false;
	dt.tm_mon = (d / 100) % 100 - 1; if (!inrange((int8_t)(dt.tm_mon + (int8_t)1), (int8_t)1, (int8_t)12)) return false;
	dt.tm_year = d / 10000 - 1900; if (!inrange(dt.tm_year + 1900, 2000, 2100)) return false;
	dt.tm_sec = t % 100; if (!inrange((int8_t)dt.tm_sec, (int8_t)0, (int8_t)59)) return false;
	dt.tm_min = t / 100 % 100; if (!inrange((int8_t)dt.tm_min, (int8_t)0, (int8_t)59)) return false;
	dt.tm_hour = t / 10000; if (!inrange((int8_t)dt.tm_hour, (int8_t)0, (int8_t)24)) return false;
	dt.tm_isdst = 0;
	return mktime(&dt);
}

