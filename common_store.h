#include "Arduino.h"
#ifdef EnergyMonitor 
//#include "ip_addr.h"
#include "lwip/ip_addr.h"
#ifdef LOCAL_H
//#include "clock.h"
#else
//#include "clock/clock.h"
#endif // LOCAL_H

#endif

#pragma once
class ConfigStoreClass
{
public:
	typedef struct {
#ifdef EnergyMonitor 
		bool _hasData;

		uint16_t periodT1Begin, periodT2Begin; // в минутах с начала дня

		sint8 TimeZone;
		char NTP_ServerName[64];

		ip_addr_t MqttUdpServer;
		uint16_t MqttUdpPort;
		bool SendDataToNarodmon;

		int32_t EnergyCountDelta;
		uint32_t EnergyCountRaw;
		uint32_t EnergyCountT1, EnergyCountT2;
		time_t EnergyCountTime, EnergyCountTimeT1, EnergyCountTimeT2;
		uint32_t EnergyCountRawOverload;

#endif

#ifdef nondef
		char AdminName[16];
		
		char AdminPassword[16];
#endif
		uint8_t crc8;
	} StoredData_t;

	static StoredData_t Data;
	static StoredData_t DefaultData;

	static void save();
	static bool load();

#ifdef EnergyMonitor
	static void setTimeServer(const char*  timeServer);
	static void setTimeZone(sint8 timeZone);

#endif // EnergyMonitor
//	static void load_default();



#ifdef nondef

	static const char * getAdminName() { return Data.AdminName; };
	static void setAdminName(const char * newAdminName);
	static const char * getAdminPassword() { return Data.AdminPassword; };
	static void setAdminPassword(const char * newAdminPassword);

	static const bool getDemoMode();// { return DataNonStored.DemoMode; };
	//static void setDemoMode(bool newDemoMode) { DataNonStored.DemoMode = newDemoMode; };
#endif

};

typedef struct {
	uint32_t energy, EnergyCount, EnergyCountT1, EnergyCountT2;
	uint16_t voltage_x10, current_x100, power;
	time_t timeLabel;
} EnergyMonitorData_t;

extern EnergyMonitorData_t EnergyMonitorData;


extern ConfigStoreClass ConfigStore;