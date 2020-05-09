// OneWireSensors.h

#ifndef _ONEWIRESENSORS_h
#define _ONEWIRESENSORS_h

#include "Arduino.h"


class OneWireSensorsClass
{
	enum SensorsTypes { // Model IDs
		DS18S20MODEL = 0x10,  // also DS1820
		DS18B20MODEL = 0x28,
		DS1822MODEL  = 0x22,
		DS1825MODEL  = 0x3B
	};
	enum OneWireCommands { // OneWire commands
		STARTCONVO      = 0x44,  // Tells device to take a temperature reading and put it on the scratchpad
		COPYSCRATCH     = 0x48,  // Copy EEPROM
		READSCRATCH     = 0xBE,  // Read EEPROM
		WRITESCRATCH    = 0x4E,  // Write to EEPROM
		RECALLSCRATCH   = 0xB8,  // Reload from last known
		READPOWERSUPPLY = 0xB4,  // Determine if device needs parasite power
		ALARMSEARCH     = 0xEC  // Query bus for devices with an alarm condition
	};


 protected:
	 static void doLoop();


 public:
	static void init();

	struct SensorData {
			uint16_t id;

		int16_t temperature;
		unsigned long time_label;
		time_t time;

		bool wasRead;

		int32_t sumTemperature;
		uint16_t countTemperature;
		int16_t getAvTemperature() { uint16_t av = (countTemperature ? sumTemperature / countTemperature : 0); sumTemperature = 0; countTemperature = 0; return av; }
		SensorData() { getAvTemperature(); };
	};

private:
	enum Settings : uint8_t {
		max_sensors = 20
	};

	static SensorData sensors_data[Settings::max_sensors];

	static uint8_t sensor_data_count;

public:

	static inline uint8_t data_count() { return sensor_data_count; };
	static bool getSensorData(uint8_t pos, SensorData& data);
	static bool getSensorData(uint16_t id, SensorData& data);
	static bool get_data_representation(uint16_t id, String & str, bool onlyNew = false);
	static bool get_data_representation(uint8_t pos, String & str, bool onlyNew = false);
	static bool get_data_representation(SensorData& data, String & str, bool onlyNew = false);

};

extern OneWireSensorsClass OneWireSensors;

#endif

