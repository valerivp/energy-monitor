#ifndef PZEM004T_H
#define PZEM004T_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//#include <SoftwareSerial.h>


class PZEM004Tnb
{
private:
	uint16_t nVoltage_x10, nCurrent_x100;
	uint16_t nPower;
	uint32_t nEnergy;

	uint32_t sumVoltage_x10, sumCurrent_x100;
	uint32_t sumPower;
	uint16_t countVoltage_x10, countCurrent_x100;
	uint16_t countPower;



public:
	PZEM004Tnb(Stream * port);
	void init(Stream * port);// uint8_t receivePin, uint8_t transmitPin);

	uint16_t getVoltage_x10() { return nVoltage_x10; };
	uint16_t getCurrent_x100() { return nCurrent_x100; };
	uint16_t getPower() { return nPower; };
	uint32_t getEnergy() { return nEnergy; };

	uint16_t getAvVoltage_x10() { uint16_t av = (countVoltage_x10 ? sumVoltage_x10 / countVoltage_x10 : 0); sumVoltage_x10 = 0; countVoltage_x10 = 0; return av; };
	uint16_t getAvCurrent_x100() { uint16_t av = (countCurrent_x100 ? sumCurrent_x100 / countCurrent_x100 : 0); sumCurrent_x100 = 0; countCurrent_x100 = 0; return av; };
	uint16_t getAvPower() { uint16_t av = (countPower ? sumPower / countPower : 0); sumPower = 0; countPower = 0; return av; };


    void setAddress(byte a1, byte a2, byte a3, byte a4, bool write = false);

	bool updateData(byte flag);
	bool isDataUpdated();

	void onReceiveByte(uint8_t c);
	void doLoop();

	enum flags {
		voltage = 0b1000,
		current = 0b0100,
		power = 0b0010,
		energy = 0b0001,
		all = 0b1111,
		none = 0
	};


private:
	struct PZEMCommand {
		uint8_t command;
		uint8_t data[5];
		uint8_t crc;
	};
	union PSEMDATA {
		PZEMCommand cmd;
		uint8_t data[sizeof(PZEMCommand)];

		void clear() { for (int8_t i = 0; i < sizeof(PZEMCommand); i++)	data[i] = 0; }
		uint8_t crc()
		{
			uint16_t tcrc = 0;
			for (uint8_t i = 0; i < sizeof(PZEMCommand) - 1; i++)
				tcrc += data[i];
			return (uint8_t)(tcrc & 0xFF);
		}
	};
	PSEMDATA ReceiverBuf, SendBuf;
	int8_t buffer_pos;
	unsigned long lastReadTime;
	unsigned long lastSendTime;

	Stream *serial;
	byte dataStatus;
	byte dataForRead;


    void send(uint8_t cmd);
 };

#endif // PZEM004T_H
