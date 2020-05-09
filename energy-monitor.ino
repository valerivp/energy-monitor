#define EnergyMonitor

#include "common_def.h"

#include "timers\timers.h"

#ifdef USE_CPU_LOAD_INFO
#include "cpuloadinfo.h"
#endif // USE_CPU_LOAD_INFO

#include "onewire\OneWire.h"

#include "esp_def.h"


#if defined(ESP8266)
#include <FS.h>
#elif defined(ESP32)
#include "SPIFFS.h"
#endif // ESP


#include <EEPROM.h>

#include "server.h"
#ifdef USE_BITLASH
#include "bitlash.hpp"
#endif // USE_BITLASH

#define LOCAL_H
#include "clock\clock.h"

#include "PZEM004T\PZEM004Tnb.h"

#include "OneWireSensors\OneWireSensors.h"

#include "common_store.h"

#include "mqtt_udp\mqtt_udp.h"
//#include <UIPEthernet.h>
#include "SoftwareSerial.h"
#undef LOCAL_H
#include <WiFiUdp.h>

//EthernetUDP udp;

SoftwareSerial swSer(pins::d6, pins::d7);
PZEM004Tnb Pzem004t(&swSer);
char strDeviceID[32];

OneWire ds18b20(pins::d5);

#ifdef USE_WEBSOCKETS
String TemperatureDataForPublic;
#endif
String EnergyDataForPublic;



void onWiFiEvent(WiFiEvent_t event)
{
#if defined(ESP8266)
	if (WiFiEvent_t::WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED != event) {
		DEBUG_PRINT(F("WiFiEvent:") << event);
	}
	switch (event)
	{
		case WiFiEvent_t::WIFI_EVENT_STAMODE_CONNECTED: {
			DEBUG_PRINT(F("Connected to: ") << WiFi.SSID());
			WiFi.enableAP(false);
			WiFi.setAutoReconnect(true);
		}break;

	default:
		break;
	}
#elif defined(ESP32)
	DEBUG_PRINT(F("WiFiEvent:") << event);

#endif

}

void initWiFi() {
	WiFi.mode(WIFI_STA);
	WiFi.onEvent(onWiFiEvent);
}

int ledState = 0;
uint16_t ledMask;

void setLedState(int newState) {
	static uint16_t ledMasks[] = { 0b0000000000000001, 0b1111111111111110, 0b0000000011111111,
		0b1010101010101010, 0b1110111011100000 };
	ledState = newState;
	ledMask = ledMasks[ledState];
}

void checkButtonAndBlinkLed() {
	static uint16_t count = 0;
	if (!digitalRead(pins::button)) {
		count++;
		digitalWrite(pins::led, count % 10);

	}else{
		if (count) {
			DEBUG_PRINT("Button pressed: " << count);
			if (count < 2) {
				// ��� ������� ���������
			}else if (count > 100) {
				DEBUG_PRINT("Restart...");
				// restart
				ESP.restart();

			}else if (count > 50) {
				DEBUG_PRINT("Change WiFi mode...");
				String ssidAP = AP_NAME_PREFIX + ESP_getChipIdStr();
				WiFi.softAP(ssidAP.c_str(), "");
				WiFi.mode(WIFI_AP_STA);
				DEBUG_PRINT("Enable AP: " << ssidAP.c_str() << ", IP: " << WiFi.softAPIP().toString());

			}else if (count > 20) {
				DEBUG_PRINT("Reset energy count");
				setInitDataPzem004tEnergyCount(ConfigStore.Data.periodT1Begin,
					ConfigStore.Data.periodT2Begin, 0, 0);

				
			}
			else if (count < 20) {
				// ����������� ����
				//setSonoffRelayState(!getSonoffRelayState());
			}

			count = 0;
		}
		if (WiFi.getMode() != WIFI_STA) {
			setLedState(2);
		};
		digitalWrite(pins::led, (ledMask & 1));
		ledMask = (ledMask >> 1) | (ledMask << 15);
	}


}

void resetLedState()
{
	if (ledState >= 3)
		setLedState(1);
}

const char * toFixed(int32_t v, int f) {
	static char buf[16];
	static char * format = "%ld.%0?d";
	format[6] = '0' + f;
	int d = 1;
	for (int i = 0; i < f; i++)
		d *= 10;
	sprintf(buf, format, int32_t(v / d), int16_t(v%d));
	//DEBUG_PRINT(format << "," << f << ", " << buf);
	return buf;
}

void sendDataToNarodmon() {
	if (!ConfigStore.Data.SendDataToNarodmon)
		return;
	if (!Clock.issync())
		return;
	

	String dataForSend("#");
	dataForSend += WiFi.macAddress() + "#PZEM004T+DS18B20\n";

	char info[64];
	for (uint8_t i = 0; i < OneWireSensors.data_count(); i++) {
		OneWireSensorsClass::SensorData data;
		if (!OneWireSensors.getSensorData(i, data))
			return;

		sprintf(info, "#0x%04x-TEMP#%s\n", data.id, toFixed(data.getAvTemperature(), 1));
		dataForSend += info;
	}

	sprintf(info, "#U1#%s\n", toFixed(Pzem004t.getAvVoltage_x10(), 1));
	dataForSend += info;
	sprintf(info, "#I1#%s\n", toFixed(Pzem004t.getAvCurrent_x100(), 2));
	dataForSend += info;
	sprintf(info, "#W1#%s\n", toFixed(Pzem004t.getAvPower(), 0));
	dataForSend += info;
	sprintf(info, "#WH1#%s\n", toFixed(EnergyMonitorData.EnergyCount, 3));
	dataForSend += info;
	sprintf(info, "#WH2#%s\n", toFixed(EnergyMonitorData.EnergyCountT1, 3));
	dataForSend += info;
	sprintf(info, "#WH3#%s\n", toFixed(EnergyMonitorData.EnergyCountT2, 3));
	dataForSend += info;

	dataForSend += "##";

	DEBUG_PRINT(endl << dataForSend);

	//return;

	WiFiUDP udp;
	udp.beginPacket("narodmon.ru", 8283);
	udp.write(dataForSend.c_str(), dataForSend.length());
	udp.endPacket();
	udp.stop();
}


void sendTemperature() {
#ifdef USE_WEBSOCKETS
	TemperatureDataForPublic = "";
#endif
	for (uint8_t i = 0; i < OneWireSensors.data_count(); i++) {
		OneWireSensorsClass::SensorData data;
		if (!OneWireSensors.getSensorData(i, data) || data.wasRead)
			return;
		data.wasRead = true;
		char topic[32];
		sprintf(topic, "DS18B20/0x%04x", data.id);
		char load[512];
		sprintf(load, "{"
			"\"timelabel\":\"%s\", "
			"\"temperature\":%d}",
			time2str(data.time),
			data.temperature);
#ifdef USE_WEBSOCKETS
		TemperatureDataForPublic += topic;
		TemperatureDataForPublic += ":";
		TemperatureDataForPublic += load;
		TemperatureDataForPublic += "\n";
#endif
		if (ConfigStore.Data.MqttUdpServer.addr) {
			mqtt_udp_send(0, topic, load);
		}
	}
}

void sendActualData() {
	if (EnergyDataForPublic.length()) {
		if (ConfigStore.Data.MqttUdpServer.addr) {
			mqtt_udp_send(0, strDeviceID, (char *)EnergyDataForPublic.c_str());
		}

		sendTemperature();

#ifdef USE_WEBSOCKETS
		TemperatureDataForPublic += strDeviceID;
		TemperatureDataForPublic += ":";
		TemperatureDataForPublic += EnergyDataForPublic;
		TemperatureDataForPublic += "\n";

		HTTPserver.wsSendText(TemperatureDataForPublic);
#endif

		EnergyDataForPublic = "";
		TemperatureDataForPublic = "";
	}
}


bool IsPeriodT1(time_t& now)
{
	tm* timestruct = localtime(&now);
	uint16_t dayTime = timestruct->tm_min + timestruct->tm_hour * 100;
	return ConfigStore.Data.periodT1Begin <= dayTime && dayTime < ConfigStore.Data.periodT2Begin;
}

void setInitDataPzem004tEnergyCount(uint16_t periodT1Begin, uint16_t periodT2Begin,
	uint32_t EnergyCountT1, uint32_t EnergyCountT2)
{
	if (!Clock.issync())
		return;

	time_t now = Clock.now();
	uint32_t energyCountRaw = Pzem004t.getEnergy();
	uint32_t EnergyCount = EnergyCountT1 + EnergyCountT2;
	ConfigStore.Data.EnergyCountRaw = energyCountRaw;
	ConfigStore.Data.EnergyCountDelta = EnergyCount - energyCountRaw;
	ConfigStore.Data.periodT1Begin = periodT1Begin;
	ConfigStore.Data.periodT2Begin = periodT2Begin;

	bool isPeriodT1 = IsPeriodT1(now);

	ConfigStore.Data.EnergyCountTimeT1 = now - (isPeriodT1 ? 1 : 0);
	ConfigStore.Data.EnergyCountT1 = EnergyCountT1;

	ConfigStore.Data.EnergyCountTimeT2 = now - (isPeriodT1 ? 0 : 1);
	ConfigStore.Data.EnergyCountT2 = EnergyCountT2;

	ConfigStore.save();

	recalcPzem004tEnergyCount();

	//DEBUG_PRINT("EnergyCountT2:" << EnergyCountT2);

}

void recalcPzem004tEnergyCount()
{
	bool ConfigStoreChanded = false;
	if (!Clock.issync())
		return;

	uint32_t energyCountRaw = Pzem004t.getEnergy();
	if (!energyCountRaw)
		return;
	if (energyCountRaw < ConfigStore.Data.EnergyCountRaw) {
		ConfigStore.Data.EnergyCountDelta += 10000000;
		ConfigStore.Data.EnergyCountRawOverload = energyCountRaw;
		ConfigStoreChanded = true;
	}
	uint32_t energyCount = energyCountRaw + ConfigStore.Data.EnergyCountDelta;

	time_t now = Clock.now();
	bool isPeriodT1 = IsPeriodT1(now);
	if (ConfigStore.Data.EnergyCountTimeT1 > ConfigStore.Data.EnergyCountTimeT2) {
		// ������ ���� ������ �2
		if (isPeriodT1) {
			ConfigStore.Data.EnergyCountTimeT2 = now;
			ConfigStore.Data.EnergyCountT2 = energyCount - ConfigStore.Data.EnergyCountT1;
			ConfigStore.Data.EnergyCountRaw = energyCountRaw;
			ConfigStoreChanded = true;
		}
	}else{
		// ������ ���� ������ �1
		if (!isPeriodT1) {
			ConfigStore.Data.EnergyCountTimeT1 = now;
			ConfigStore.Data.EnergyCountT1 = energyCount - ConfigStore.Data.EnergyCountT2;
			ConfigStore.Data.EnergyCountRaw = energyCountRaw;
			ConfigStoreChanded = true;
		}
	}
	if (ConfigStoreChanded)
		ConfigStore.save();


	EnergyMonitorData.EnergyCount = energyCount;
	//DEBUG_PRINT("EnergyCount:" << EnergyCount);
	if (isPeriodT1) {
		EnergyMonitorData.EnergyCountT1 = energyCount - ConfigStore.Data.EnergyCountT2;
		EnergyMonitorData.EnergyCountT2 = ConfigStore.Data.EnergyCountT2;
	}else{
		EnergyMonitorData.EnergyCountT1 = ConfigStore.Data.EnergyCountT1;
		EnergyMonitorData.EnergyCountT2 = energyCount - ConfigStore.Data.EnergyCountT1;
	}
}

void readPzem004t() {
	static PZEM004Tnb::flags flags = (PZEM004Tnb::flags)(PZEM004Tnb::flags::all ^ PZEM004Tnb::flags::energy);
	static unsigned long lastReadEnergyTime = 0;

	if (Pzem004t.isDataUpdated()) {
		setLedState(3);
		Timers.once(resetLedState, 700); // min 700
		EnergyMonitorData.timeLabel = Clock.now();
		EnergyMonitorData.voltage_x10 = Pzem004t.getVoltage_x10();
		EnergyMonitorData.current_x100 = Pzem004t.getCurrent_x100();
		EnergyMonitorData.power = Pzem004t.getPower();
		EnergyMonitorData.energy = Pzem004t.getEnergy();
		if (flags & PZEM004Tnb::flags::energy) {
			Serial << endl;
			Serial << "Voltage: " << Pzem004t.getVoltage_x10() << endl;
			Serial << "Current: " << Pzem004t.getCurrent_x100() << endl;
			Serial << "Power: " << Pzem004t.getPower() << endl;
			Serial << "Energy: " << Pzem004t.getEnergy() << endl;

			recalcPzem004tEnergyCount();
		}

		//DEBUG_PRINT(ConfigStore.Data.MqttUdpServer.addr);
		char load[512];
		sprintf(load, "{"
			"\"timelabel\":\"%s\", "
			"\"voltage\":%d, "
			"\"current\":%d, "
			"\"power\":%d, "
			"\"energy\":%ld, "
			"\"energyT1\":%ld, "
			"\"energyT2\":%ld}",
			time2str(EnergyMonitorData.timeLabel),
			EnergyMonitorData.voltage_x10,
			EnergyMonitorData.current_x100,
			EnergyMonitorData.power,
			EnergyMonitorData.EnergyCount,
			EnergyMonitorData.EnergyCountT1,
			EnergyMonitorData.EnergyCountT2);

		EnergyDataForPublic = load;

		unsigned long currentTime = millis();
		if (Clock.issync() && ((!lastReadEnergyTime) || ((currentTime - lastReadEnergyTime) > 60000))) { // ��� � 1 ��� ������ �������
			lastReadEnergyTime = currentTime;
			flags = PZEM004Tnb::flags::all;
		}else{
			flags = (PZEM004Tnb::flags)(PZEM004Tnb::flags::all ^ PZEM004Tnb::flags::energy);
		}
	}
	Pzem004t.updateData(flags);
}

int mqtt_udp_send_pkt(int fd, char *data, size_t len)
{
	WiFiUDP udp;
	//success = udp.beginPacket(IPAddress(192, 168, 1, 255), ConfigStore.Data.MqttUdpPort);
	udp.beginPacket(IPAddress(ConfigStore.Data.MqttUdpServer.addr), ConfigStore.Data.MqttUdpPort);
	udp.write(data, len);
	udp.endPacket();
	udp.stop();
	//Serial.println("sent!");
	return 0;
}

void DelayedInitPzem004()
{
	swSer.begin(9600);
	Pzem004t.setAddress(192, 168, 1, 1);
	Timers.add(readPzem004t, 100, F("readPzem004t"));
}

void setup(void) {
	ESP_wdtEnable(10000);

	Serial.begin(115200); Serial.println(); // ��� ������ � ����� ����� ���� �����, ������ � ������� �����

	DEBUG_PRINT("Started");
	DEBUG_PRINT("Version: " << __TIMESTAMP__);

	pinMode(pins::led, OUTPUT);

	pinMode(pins::button, INPUT_PULLUP);

	ConfigStore.load();

	initWiFi();

	SPIFFS.begin();

	Clock.init(ConfigStore.Data.NTP_ServerName, ConfigStore.Data.TimeZone);

	Timers.add(checkButtonAndBlinkLed, 100, F("checkButtonAndBlinkLed"));

	EnergyDataForPublic.reserve(256);
	TemperatureDataForPublic.reserve(2048);
	uint32_t id = ESP.getChipId();
	sprintf(strDeviceID, "Pzem004t/0x%04x", crc16((uint8_t*)&id, 4));
	Timers.once(DelayedInitPzem004, 1000);
	setLedState(4);

	OneWireSensors.init();
	//Timers.add(sendTemperature, 2000, F("sendTemperature"));

	HTTPserver.init();

	Timers.add(sendDataToNarodmon, 5 * 60 * 1000, F("sendDataToNarodmon"));
	Timers.add(sendActualData, 100, F("sendActualData"));
}





void loop(void) {
	static unsigned long lastTime = 0;
	unsigned long currTime = millis();

	if ((currTime - lastTime) > 1000)
		DEBUG_PRINT(F("No loop more than one second"));
	lastTime = currTime;

	ESP_wdtFeed();


	Timers.doLoop();

	if(swSer.available()) {
		uint8_t b = swSer.read();
		Pzem004t.onReceiveByte(b);
	}
	Pzem004t.doLoop();

	if ((millis() - lastTime) > 1000)
		DEBUG_PRINT(F("It was more than one second: Timers.doLoop"));


}
