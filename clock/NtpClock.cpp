#include "clock.h"
#include "../Timers/timers.h"
#include "../debug.h"
#include <ESP8266WiFi.h>
#include <FS.h>



NtpClockClass Clock;

// NTP Servers:
IPAddress NtpClockClass::TimeServerIP;// (132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov

WiFiUDP NtpClockClass::udp;
NtpClockClass::TimeZones NtpClockClass::TimeZone(NtpClockClass::TimeZones::UTCp3);
String NtpClockClass::TimeServerName = "ntp1.stratum2.ru";

time_t NtpClockClass::getTimeNTP()
{
	DEBUG_STACK;
	const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
	byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
	static IPAddress emptyAddress;
	if (TimeServerIP == emptyAddress)
		return 0;

	while (udp.parsePacket() > 0); // discard any previously received packets
	DEBUG_PRINT("Transmit NTP Request");

	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	//bzero(packetBuffer, NTP_PACKET_SIZE);

	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
							 // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:                 
	udp.beginPacket(TimeServerIP, 123); //NTP requests are to port 123
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();

	uint32_t beginWait = millis();
	while (millis() - beginWait < 1000) {
		int size = udp.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			DEBUG_PRINT("Receive NTP Response");
			udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
			unsigned long secsSince1900;
			// convert four bytes starting at location 40 to a long integer
			secsSince1900 = (unsigned long)packetBuffer[40] << 24;
			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
			secsSince1900 |= (unsigned long)packetBuffer[43];
			return secsSince1900 - 2208988800UL;// +TimeZone * SECS_PER_HOUR;
		}
	}
	DEBUG_PRINT("No NTP Response :-(");
	return 0; // return 0 if unable to get the time
}

void NtpClockClass::init(SyncProviders SyncProvider)
{
	DEBUG_STACK;

	loadConfig();

	udp.begin(123);

	Timers.add(doLoop, 250, ("" __FILE__ ": NtpClockClass::doLoop"));
	Timers.add(setSyncInterval, 20000, ("" __FILE__ ": NtpClockClass::setSyncInterval"));
	switch (SyncProvider)
	{
		case NtpClockClass::NTP:
			setSyncProvider(getTimeNTP);
		break;
	default:
		break;
	}
}

void NtpClockClass::setTimeZone(TimeZones timeZone)
{
	TimeZone = timeZone;
	saveConfig();
	loadConfig();
}
NtpClockClass::TimeZones NtpClockClass::getTimeZone()
{
	return TimeZone;
}
void NtpClockClass::setTimeServer(String & timeServer)
{
	TimeServerName = timeServer;
	saveConfig();
	loadConfig();
}
String NtpClockClass::getTimeServer()
{
	return TimeServerName;
}


void NtpClockClass::saveConfig()
{
	DEBUG_STACK;
	File file = SPIFFS.open("/ntp.cfg", "w");
	file.print(TimeServerName);
	file.print('\n');
	file.print(TimeZone);
	file.close();
	DEBUG_PRINT("NTP server name: " << TimeServerName << ", time zone: " << TimeZone);
}


void NtpClockClass::loadConfig()
{
	DEBUG_STACK;
	if (SPIFFS.exists("/ntp.cfg")) {
		File file = SPIFFS.open("/ntp.cfg", "r");
		String config = file.readString();
		int chind = config.indexOf('\n');
		TimeServerName = config.substring(0, chind);
		TimeZone = (TimeZones)(config.substring(chind + 1)).toInt();
		file.close();
	}
	DEBUG_PRINT("NTP server name: " << TimeServerName << ", time zone: " << TimeZone);
	setTimeServerIP();
}

void NtpClockClass::setTimeServerIP()
{
	DEBUG_STACK;
	IPAddress empty;
	WiFi.hostByName(TimeServerName.c_str(), TimeServerIP);
	if (TimeServerIP == empty) {
		Timers.once(setTimeServerIP, 30000);
	}
	DEBUG_PRINT("NTP server IP: " << TimeServerIP.toString());
}


/*
const char * NtpClockClass::now_xmlstr()
{
	static char buf[28]; //2014-05-27T00:00:00
	tmElements_t tm;
	breakTime(now() + TimeZone * SECS_PER_HOUR, tm);
	os_sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d", tm.Year + 1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
	return buf;
}*/

String NtpClockClass::toXmlStr(time_t time)
{
	static char buf[28]; //2014-05-27T00:00:00+03.00
	tmElements_t tm;
	
	breakTime((time ? time: now()), tm);
	// функция не понимает %+02d, знак печатаем отдельно
	os_sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d%s%02d.00", tm.Year + 1970, tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second, (TimeZone < 0 ? "-" : "+"), TimeZone);
	return String(buf);
}

void NtpClockClass::doLoop()
{
	now();
}

void NtpClockClass::setSyncInterval() {
	if (timeStatus() == timeNotSet)
		::setSyncInterval(10);
	else
		::setSyncInterval(3600);
}



