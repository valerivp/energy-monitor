// RtcClock.h

#ifndef _RTCCLOCK_h
#define _RTCCLOCK_h

#include "Arduino.h"

#include "clock.h"
#include <Wire.h>
#include <RtcDS3231.h>




class RtcClockClass:public ClockClass
{
 protected:
	static time_t sysTime;

 public:
	static void init();
	static void sync();
	// 32-bit times as seconds since 1/1/2000
	static time_t now();
	// 32-bit times as seconds since 1/1/1970
	inline static time_t nowUNX() { return now() + SEC_1970_TO_2000; };
	static void set(time_t time);
	inline static void setUNX(time_t time) { set(time - SEC_1970_TO_2000); };
	inline static RtcDateTime getDateTime(time_t time = 0) { if (!time) time = now(); return RtcDateTime(time); };
	inline static RtcDateTime getDateTimeUNX(time_t time = 0) { if (!time) time = nowUNX(); return RtcDateTime(time - SEC_1970_TO_2000); };
};

extern RtcClockClass RtcClock;

#endif

