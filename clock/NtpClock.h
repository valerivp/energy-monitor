#ifndef CLOCK_H
#define CLOCK_H
#pragma once

#include "TimeLib.h"
#include "WiFiUdp.h"

class NtpClockClass {
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
	enum SyncProviders {
		NTP = 1
	};
private:
	//setSyncProvider(getNtpTime);
	static time_t getTimeNTP();
	static WiFiUDP udp;
	static IPAddress TimeServerIP;
	static String TimeServerName;
	static TimeZones TimeZone;
	static void setSyncInterval();
	static void saveConfig();
	static void loadConfig();
	static void setTimeServerIP();
public:
	static void init(SyncProviders SyncProvider);
	static void setTimeZone(TimeZones timeZone);
	static TimeZones getTimeZone();
	static void setTimeServer(String& timeServer);
	static String getTimeServer();
	inline static time_t now() { return ::now(); };
	//inline static time_t now_local() { return ::now() + TimeZone * SECS_PER_HOUR; };
	//static const char* now_xmlstr();
	static String toXmlStr(time_t time = 0);
	static void doLoop();

};

extern NtpClockClass Clock;


#endif
