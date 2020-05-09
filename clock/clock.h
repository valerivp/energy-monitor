// Clock.h

#ifndef _CLOCK_h
#define _CLOCK_h

//include "Arduino.h"
#include <stdint.h>
/*
#ifdef LOCAL_H
#include "common_def.h"
#else
#include "../common_def.h"
#endif // LOCAL_H
*/

extern "C" {
#include <time.h>
}


//#define USE_NTP_CLIENT


#if defined(USE_RTC_CLOCK)

#elif defined(USE_NTP_CLIENT) && defined(ESP8266)

#include <ESP8266WiFi.h>
#include <FS.h>
extern "C" {
#include "sntp.h"
}
#include "WiFiUdp.h"

#elif defined(USE_NTP_CLIENT) && defined(ESP32)
extern "C" {
#include "apps\sntp\sntp.h"
}

#else 

#endif




class ClockBaseClass
{
protected:
	enum Settings {
		MaxSyncTimeout = 2 * 24 * 60 * 60 + 5 * 60 // максимальное время между синхронизациями, для которого время считается актуальным
	};

protected:
	static time_t sysTime;
	static time_t lastSyncTime;
	static time_t upTime;
	static uint32_t prevMillis;
public:
	static void init() { sync(); };
	static bool sync() { return false; };
	static inline time_t synctime() { return lastSyncTime; };
	static bool issync();
	// return seconds elapsed from Midnight, Jan 1 2000 UTC
	static time_t now();
	static inline time_t uptime() { now(); return upTime; };
	static void setTime(time_t t) { sysTime = t;  lastSyncTime = sysTime; };
};


#if defined(USE_RTC_CLOCK)

class ClockRtcClass : public ClockBaseClass {
public:
	static bool init();
	static bool sync();
	static void setTime(time_t t);
};

extern ClockRtcClass Clock;
//extern ClockBaseClass Clock;

#elif defined(USE_NTP_CLIENT)

class ClockNtpClass : public ClockBaseClass {
public:
	enum TimeZones {
		UTCm11 = -11,
		UTCm10 = -10,
		UTCm9 = -9,
		UTCm8 = -8,
		UTCm7 = -7,
		UTCm6 = -6,
		UTCm5 = -5,
		UTCm4 = -4,
		UTCm3 = -3,
		UTCm2 = -2,
		UTCm1 = -1,
		UTC = 0,
		UTCp1 = 1,
		UTCp2 = 2,
		UTCp3 = 3,
		UTCp4 = 4,
		UTCp5 = 5,
		UTCp6 = 6,
		UTCp7 = 7,
		UTCp8 = 8,
		UTCp9 = 9,
		UTCp10 = 10,
		UTCp11 = 11
	};

protected:
	static TimeZones TimeZone;
#ifndef EnergyMonitor	
	static void loadConfig();
	static void saveConfig();
	static char * DefaultTimeServerName;
#endif
private:
	static bool init();
public:
	static bool init(const char* timeServer, sint8 timeZone);
	static bool sync();
	static void setTimeZone(TimeZones timeZone);
	static inline void setTimeZone(sint8 timeZone) { setTimeZone((TimeZones)timeZone); };
	static inline TimeZones getTimeZone() { return TimeZone; };

	static void setTimeServer(const char* timeServer);
	static char* getTimeServer();
	//static unsigned int TimeServerNameMaxLen();

};

extern ClockNtpClass Clock;

#else 
extern ClockBaseClass Clock;

#endif



#endif // _CLOCK_h
