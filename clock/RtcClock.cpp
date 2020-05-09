// 
// 
// 
#include "RtcClock.h"

RtcDS3231<TwoWire> RTC(Wire);

RtcClockClass RtcClock;

time_t RtcClockClass::sysTime = 0;

#define SEC_1970_TO_2000 946684800

void RtcClockClass::init()
{
	RTC.Begin();
	sync();
}

time_t RtcClockClass::now()
{
	static uint32_t prevMillis = 0;
	// calculate number of seconds passed since last call to now()
	while (millis() - prevMillis >= 1000) {
		// millis() and prevMillis are both unsigned ints thus the subtraction will always be the absolute value of the difference
		sysTime++;
		prevMillis += 1000;
	}
	return sysTime;
}

void RtcClockClass::set(time_t time)
{
	RtcDateTime dt(time - SEC_1970_TO_2000);
	RTC.SetDateTime(dt);
	sync();
}

void RtcClockClass::sync()
{
	RtcDateTime dt = RTC.GetDateTime();
	sysTime = dt.TotalSeconds() + SEC_1970_TO_2000;
}


