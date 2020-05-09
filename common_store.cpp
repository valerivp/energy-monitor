#include "common_def.h"
#include "common_store.h"
#include "EEPROM.h"
#include "clock/clock.h"

ConfigStoreClass ConfigStore;
EnergyMonitorData_t EnergyMonitorData;

#ifdef EnergyMonitor
ConfigStoreClass::StoredData_t ConfigStoreClass::DefaultData = { true, 7 * 100, 23 * 100, 3, "192.168.1.1", {PP_HTONL(LWIP_MAKEU32(192,168,1,1))}, 1883};
ConfigStoreClass::StoredData_t ConfigStoreClass::Data;
#else

ConfigStoreClass::StoredData_t ConfigStoreClass::DefaultData = { "", "", 0 };
ConfigStoreClass::StoredData_t ConfigStoreClass::Data = {"", "", 0};

ConfigStoreClass::NonStoredData_t ConfigStoreClass::DataNonStored = {false};
#endif

void ConfigStoreClass::save()
{
	Data.crc8 = crc8((const uint8_t *)&Data, sizeof(Data) - sizeof(Data.crc8));

	EEPROM.begin(sizeof(Data));
	EEPROM.put(0, Data);
	EEPROM.commit();
	EEPROM.end();
}

bool ConfigStoreClass::load()
{
	StoredData_t tempData;
	EEPROM.begin(sizeof(tempData));
	EEPROM.get(0, tempData);
	EEPROM.end();

	uint8_t crc = crc8((const uint8_t *)&tempData, sizeof(tempData) - sizeof(Data.crc8));
	if (!tempData._hasData || crc != tempData.crc8) {
		Data = DefaultData;
		return false;
	}else {
		Data = tempData;
		return true;
	}
}

#ifdef EnergyMonitor
void ConfigStoreClass::setTimeServer(const char * timeServer)
{
	//	DEBUG_PRINT(TimeServerNameMaxLen());
	unsigned int TimeServerNameMaxLen = sizeofArray(ConfigStore.Data.NTP_ServerName) - 1;
	memset(ConfigStore.Data.NTP_ServerName, 0, TimeServerNameMaxLen);
	strncpy(ConfigStore.Data.NTP_ServerName, timeServer, min((unsigned int)strlen(timeServer), TimeServerNameMaxLen));
	ConfigStore.save();

	Clock.setTimeServer(ConfigStore.Data.NTP_ServerName);

}

void ConfigStoreClass::setTimeZone(sint8 timeZone)
{
	ConfigStore.Data.TimeZone = timeZone;
	ConfigStore.save();

	Clock.setTimeZone(timeZone);
}
#endif // EnergyMonitor


#ifdef nodef

void ConfigStoreClass::setAdminName(const char * newAdminName)
{
	strncpy(Data.AdminName, newAdminName, sizeof(Data.AdminName) - sizeof('\0'));
	save();
}

void ConfigStoreClass::setAdminPassword(const char * newAdminPassword)
{
	strncpy(Data.AdminPassword, newAdminPassword, sizeof(Data.AdminPassword) - sizeof('\0'));
	save();
}

const bool ConfigStoreClass::getDemoMode()
{
	/* init demo mode check */
#if defined(ESP8266)

	pinMode(pins::d3, INPUT_PULLUP);
	pinMode(pins::d4, OUTPUT);
	digitalWrite(pins::d4, LOW);
	bool res = !digitalRead(pins::d3);
	digitalWrite(pins::d4, HIGH);
	return res;

#elif defined(ESP32)

	DEBUG_PRINT(F("Demo mode check not imlemented"));
	return false;
#endif

}
#endif


