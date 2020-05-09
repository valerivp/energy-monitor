#include "..\common_def.h"

//#define LOCAL_H
#include "Clock.h"
//#undef LOCAL_H

#include "../Timers/timers.h"

#if defined(USE_RTC_CLOCK)

#include "..\RTC_DS3231\RTC_DS3231.h"
ClockRtcClass Clock;
//ClockBaseClass Clock;

#elif defined(USE_NTP_CLIENT)

ClockNtpClass Clock;

// NTP Servers:
ClockNtpClass::TimeZones ClockNtpClass::TimeZone(ClockNtpClass::TimeZones::UTCp3);

#else 
ClockBaseClass Clock;
#endif


time_t ClockBaseClass::sysTime = 0;
time_t ClockBaseClass::upTime = 0;
uint32_t ClockBaseClass::prevMillis = 0;
time_t ClockBaseClass::lastSyncTime = 0;


bool ClockBaseClass::issync()
{
	time_t st = synctime();
	return st && ((uptime() - st) < Settings::MaxSyncTimeout);
}

time_t ClockBaseClass::now()
{
	//return time(NULL);
	// calculate number of seconds passed since last call to now()
	while (millis() - prevMillis >= 1000) {
		// millis() and prevMillis are both unsigned ints thus the subtraction will always be the absolute value of the difference
		sysTime++;
		upTime++;
		prevMillis += 1000;
	}
	return sysTime;
}



#if defined(USE_RTC_CLOCK)

bool ClockRtcClass::init()
{
	RTC_DS3231.init();
	Timers.once((void(*)())sync, 1000);
	Timers.add((void(*)())sync, 1000 * Settings::MaxSyncTimeout / 10, F("ClockRtc::sync"));

	return true;
}

bool ClockRtcClass::sync()
{
	time_t newtime = RTC_DS3231.getTime();
	if (newtime) {
		sysTime = newtime;

		prevMillis = millis();
		lastSyncTime = uptime();

		return true;
	}
	return false;
}

void ClockRtcClass::setTime(time_t t)
{
	RTC_DS3231.setTime(t);
	sync();
}


#elif defined(USE_NTP_CLIENT)

bool ClockNtpClass::init()
{
#if defined(ESP32)
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
#endif

	sntp_setservername(1, "ntp1.stratum2.ru");
	sntp_setservername(2, "pool.ntp.org");

	sntp_init();


	Timers.once((void(*)())sync, 500);
	Timers.add((void(*)())sync, 1000 * Settings::MaxSyncTimeout / 10, F("ClockNtp::sync"));

	return true;
}

bool ClockNtpClass::init(const char * timeServer, sint8 timeZone)
{
	setTimeServer(timeServer);
	setTimeZone(timeZone);
	init();
	return true;
}


#ifndef EnergyMonitor
void ClockNtpClass::saveConfig()
{

	/*
	//DEBUG_STACK();
	File file = SPIFFS.open("/ntp.cfg", "w");
	file.print(sntp_getservername(0));
	file.print('\n');
#if defined(ESP8266)
	file.print(sntp_get_timezone());
	DEBUG_PRINT("NTP server name: " << sntp_getservername(0) << ", time zone: " << sntp_get_timezone());
#elif defined(ESP32)

#endif
	file.close();
	*/
}

void ClockNtpClass::loadConfig()
{
	//DEBUG_STACK();
	if (SPIFFS.exists("/ntp.cfg")) {
		File file = SPIFFS.open("/ntp.cfg", "r");
		String config = file.readString();
		int chind = config.indexOf('\n');
		String ts = config.substring(0, chind);
		setTimeServer(ts);
		setTimeZone((TimeZones)(config.substring(chind + 1)).toInt());
		file.close();
	}
}
#endif


bool ClockNtpClass::sync()
{
	//DEBUG_STACK();
#if defined(ESP8266)
	uint32 ts = sntp_get_current_timestamp();
	DEBUG_PRINT(ts);
#elif defined(ESP32)
	time_t ts = time(nullptr);
#endif
	if (ts > 946684800){ // 01.01.2000
		sysTime = ts;// +TimeZone * 60 * 60;
		prevMillis = millis();
		lastSyncTime = uptime();
		//DEBUG_PRINT(now() << ":" << time2str(now()));
		return true;
	}
	Timers.once((void(*)())sync, 3000);
	return  false;
}

void ClockNtpClass::setTimeZone(TimeZones timeZone)
{
	TimeZone = timeZone;
	sntp_set_timezone(timeZone);

}

void ClockNtpClass::setTimeServer(const char*  timeServer)
{
	//IPAddress ip;
/*	u32 ip_u32 = ipaddr_addr(timeServer);
	if (ip_u32) {
//	if (ip.fromString(timeServer)) {
		ip_addr_t _ip = IPADDR4_INIT(ip_u32);
		sntp_setserver(0, &_ip);
	}else
*/		sntp_setservername(0, (char*)timeServer);

}

char * ClockNtpClass::getTimeServer()
{
/*	ip_addr_t _ip = sntp_getserver(0);
	if (_ip.addr) {
		return ipaddr_ntoa(&_ip);
	}else
*/		return sntp_getservername(0);
}
/*
unsigned int ClockNtpClass::TimeServerNameMaxLen()
{
	return sizeofArray(ConfigStore.Data.NTP_ServerName) - 1;
}

*/
#endif 