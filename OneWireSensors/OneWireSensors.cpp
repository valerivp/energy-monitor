
#include "../OneWire/OneWire.h"

#include "OneWireSensors.h"

#include "../Timers/timers.h"

#include "..\common_def.h"
#include "../Clock/clock.h"



OneWireSensorsClass OneWireSensors;
uint8_t OneWireSensorsClass::sensor_data_count = 0;
OneWireSensorsClass::SensorData OneWireSensorsClass::sensors_data[];

OneWire oneWire(pins::d5);

void OneWireSensorsClass::init()
{
	Timers.add(doLoop, 1000, F("OneWireSensorsClass::doLoop"));
}


void OneWireSensorsClass::doLoop()
{
	uint8_t addr[8];
	if (!oneWire.search(addr)) {
		//DEBUG_PRINT("search");
		oneWire.reset_search();
		oneWire.reset();
		oneWire.skip();
		oneWire.write(OneWireCommands::STARTCONVO);
		return;
	}
	//DEBUG_PRINT("found");
	if (oneWire.crc8(addr, 7) != addr[7]) {
		String tmp;
		for (int8_t i = 0; i < sizeof(addr); i++) {
			tmp += toHex(addr[i]);
			tmp += ' ';
		}
		DEBUG_PRINT(F("Addr CRC is not valid: ") << tmp);
		return;
	}
	// найден датчик, узнаем температуру
	oneWire.reset();
	oneWire.select(addr);
	oneWire.write(OneWireCommands::READSCRATCH);
	uint8_t databuf[9];
	for (int8_t i = 0; i < sizeof(databuf); i++) {           // we need 9 bytes
		databuf[i] = oneWire.read();
	}
	if (oneWire.crc8(databuf, 8) != databuf[8]) {
		String tmp;
		for (int8_t i = 0; i < sizeof(databuf); i++) {
			tmp += toHex(databuf[i]);
			tmp += ' ';
		};
		DEBUG_PRINT(F("Data CRC is not valid: ") << tmp);
			return;
	}

	SensorData data;
	data.id = crc16(addr, sizeof(addr));
	data.time_label = millis();
	data.time = Clock.now();
	data.wasRead = false;

	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (databuf[1] << 8) | databuf[0];
	
	byte cfg = (databuf[4] & 0x60);
	// at lower res, the low bits are undefined, so let's zero them
	if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
	else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
	else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
											//// default is 12 bit resolution, 750 ms conversion time
	data.temperature = (raw*10) >> 4;
	data.sumTemperature += data.temperature;
	data.countTemperature++; if (!data.countTemperature) data.sumTemperature = 0;

	int sensor_index = 0;
	for (; sensor_index < sensor_data_count; sensor_index++) {
		if (data.id == sensors_data[sensor_index].id) { // нашли данные датчика
			sensors_data[sensor_index] = data;
			break;
		}
	}
	if (sensor_index >= sensor_data_count) { // не нашли данные датчка
		if (sensor_data_count < max_sensors) { // датчиков меньше возможного, добавляем новый
			sensors_data[sensor_data_count] = data;
			sensor_data_count++;
		}
		else { //надо заместить существующий, редкая ситуация, отдельный поиск
			unsigned long max_difference = 0;
			int first_item = -1;
			for (int i = 0; i < sensor_data_count; i++) {
				unsigned long difference = data.time_label - sensors_data[i].time_label;
				if (max_difference < difference) { // ищем наиболее ранние показатели
					first_item = i;
					max_difference = difference;
				}
			}
			sensors_data[first_item] = data;
		}
	}

}



bool OneWireSensorsClass::getSensorData(uint8_t pos, SensorData& data)
{
	if (pos >= data_count())
		return false;
	data = sensors_data[pos];
	sensors_data[pos].wasRead = true;
	return true;
}

bool OneWireSensorsClass::getSensorData(uint16_t id, SensorData& data)
{
	for (uint8_t i = 0; i < data_count(); i++) {
		if (sensors_data[i].id == id)
			return getSensorData(i, data);
	}
	return false;
}

bool OneWireSensorsClass::get_data_representation(uint16_t id, String & str, bool onlyNew)
{
	SensorData data;
	if (!getSensorData(id, data))
		return false;

	return get_data_representation(data, str, onlyNew);
}

bool OneWireSensorsClass::get_data_representation(uint8_t pos, String & str, bool onlyNew)
{
	SensorData data;
	if (!getSensorData(pos, data))
		return false;
	return get_data_representation(data, str, onlyNew);

}

bool OneWireSensorsClass::get_data_representation(SensorData & data, String & str, bool onlyNew)
{
	if (onlyNew && data.wasRead)
		return false;

	str = String(F("sensor:0x")) + toHex(data.id) + F(",type:DS18B20,temperature:") + data.temperature
		+ F(",uptime_label:") + data.time_label + F(",time_label:") + time2str(data.time);

	return true;
}
