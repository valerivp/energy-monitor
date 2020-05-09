#include "PZEM004Tnb.h"

#define PZEM_VOLTAGE (uint8_t)0xB0
#define RESP_VOLTAGE (uint8_t)0xA0

#define PZEM_CURRENT (uint8_t)0xB1
#define RESP_CURRENT (uint8_t)0xA1

#define PZEM_POWER   (uint8_t)0xB2
#define RESP_POWER   (uint8_t)0xA2

#define PZEM_ENERGY  (uint8_t)0xB3
#define RESP_ENERGY  (uint8_t)0xA3

#define PZEM_SET_ADDRESS (uint8_t)0xB4
#define RESP_SET_ADDRESS (uint8_t)0xA4

#define PZEM_POWER_ALARM (uint8_t)0xB5
#define RESP_POWER_ALARM (uint8_t)0xA5

#define RESPONSE_SIZE sizeof(PZEMCommand)
//#define RESPONSE_DATA_SIZE RESPONSE_SIZE - 2

#define PZEM_DEFAULT_READ_TIMEOUT 1000 // min 600. 1000 - не хочу получать данные чаше чем 3 сек
#define PZEM_DEFAULT_SEND_TIMEOUT 2000

//#define PZEM_ERROR_VALUE -1.0



PZEM004Tnb::PZEM004Tnb(Stream * port)//uint8_t receivePin, uint8_t transmitPin)
{
  //  SoftwareSerial *port = new SoftwareSerial(receivePin, transmitPin);
    //port->begin(PZEM_BAUD_RATE);
    this->serial = port;

	nVoltage_x10 = 0, nCurrent_x100 = 0, nPower = 0;
	nEnergy = 0;

	dataStatus = flags::all;
	dataForRead = flags::none;

	SendBuf.clear();
	ReceiverBuf.clear();
	buffer_pos = 0;
	lastReadTime = 0;
	lastSendTime = 0;

	getAvVoltage_x10();
	getAvCurrent_x100();
	getAvPower();
}



void PZEM004Tnb::init(Stream * port)//uint8_t receivePin, uint8_t transmitPin)
{
	//port->begin(PZEM_BAUD_RATE);
	this->serial = port;
}

void PZEM004Tnb::setAddress(byte a1, byte a2, byte a3, byte a4, bool write)
{
	SendBuf.cmd.data[0] = a1, SendBuf.cmd.data[1] = a2, SendBuf.cmd.data[2] = a3, SendBuf.cmd.data[3] = a4;
	if(write)
		send(PZEM_SET_ADDRESS);
}

bool PZEM004Tnb::updateData(byte flag)
{
	if (isDataUpdated() || (millis() - lastSendTime) >= PZEM_DEFAULT_SEND_TIMEOUT) {
		dataStatus = flags::all & flag;
		dataForRead = dataStatus;
		//Serial.println("uT");
		return true;
	}
	//Serial.println("uF");
	return false;
}

bool PZEM004Tnb::isDataUpdated()
{
	return !dataStatus;
}



void PZEM004Tnb::onReceiveByte(uint8_t c)
{
	unsigned long currentTime = millis();
	if ((currentTime - lastReadTime) > PZEM_DEFAULT_READ_TIMEOUT) {
		ReceiverBuf.clear();
		buffer_pos = 0;
		//DEBUG_PRINT("Clear buffer");
	}
	lastReadTime = currentTime;
	if (buffer_pos == sizeof(ReceiverBuf)) {
		//DEBUG_PRINT("Move buffer");
		for (int8_t i = 1; i < sizeof(ReceiverBuf); i++)
			ReceiverBuf.data[i - 1] = ReceiverBuf.data[i];
		buffer_pos = sizeof(ReceiverBuf) - 1;
	}
	ReceiverBuf.data[buffer_pos] = c;
	buffer_pos++;
	/*if (buffer_pos == sizeof(ReceiverBuf) ) {
	DEBUG_PRINT("buffer_pos: " << buffer_pos << ", ReceiverBuf.cmd.crc: " << ReceiverBuf.cmd.crc << ", ReceiverBuf.crc(): " << ReceiverBuf.crc());
	}*/
	if (buffer_pos == sizeof(ReceiverBuf) && ReceiverBuf.cmd.crc == ReceiverBuf.crc()) {
		//Serial.println(ReceiverBuf.cmd.command);
		//Serial.print("r");
		//Serial.println(ReceiverBuf.cmd.command, HEX);
		uint8_t *data = ReceiverBuf.cmd.data;
		switch (ReceiverBuf.cmd.command)
		{
			case RESP_VOLTAGE: {
				dataStatus &= flags::all ^ flags::voltage;
				nVoltage_x10 = ((uint16_t(data[0]) << 8) + data[1]) * 10 + data[2];
				sumVoltage_x10 += nVoltage_x10;
				countVoltage_x10++; if (!countVoltage_x10) sumVoltage_x10 = 0;
				//Serial.println(nVoltage_x10);
			}break;
			case RESP_CURRENT: {
				dataStatus &= flags::all ^ flags::current;
				//nCurrent_x100 = 100 * ReceiverBuf.cmd.data[1] + ReceiverBuf.cmd.data[2];
				nCurrent_x100 = uint16_t(data[1]) * 100 + data[2];
				sumCurrent_x100 += nCurrent_x100;
				countCurrent_x100++; if (!countCurrent_x100) sumCurrent_x100 = 0;
			}break;
			case RESP_POWER: {
				dataStatus &= flags::all ^ flags::power;
				nPower = (uint16_t(data[0]) << 8) + data[1]; //256 * data[0] + data[1];
				sumPower += nPower;
				countPower++; if (!countPower) sumPower = 0;
			}break;
			case RESP_ENERGY: {
				dataStatus &= flags::all ^ flags::energy;
				nEnergy = (uint32_t(data[0]) << 16) + (uint32_t(data[1]) << 8) + data[2];
			}break;
		}
		ReceiverBuf.clear();
		buffer_pos = 0;

	}

}

void PZEM004Tnb::doLoop()
{
	unsigned long currentTime = millis();
	if (dataForRead && (currentTime - lastSendTime) > PZEM_DEFAULT_READ_TIMEOUT) {
		
		lastSendTime = currentTime;

		ReceiverBuf.clear();
		buffer_pos = 0;
		if (dataForRead & flags::voltage) {
			dataForRead ^= flags::voltage;
			send(PZEM_VOLTAGE);
		}else if (dataForRead & flags::current) {
			dataForRead ^= flags::current;
			send(PZEM_CURRENT);
		}else if (dataForRead & flags::power) {
			dataForRead ^= flags::power;
			send(PZEM_POWER);
		}else if (dataForRead & flags::energy) {
			dataForRead ^= flags::energy;
			send(PZEM_ENERGY);
		}
	}
}



void PZEM004Tnb::send(uint8_t cmd)
{
	//DEBUG_PRINT("test");
	//serial->print(1);
	SendBuf.cmd.command = cmd;

	uint8_t *bytes = SendBuf.data;
	SendBuf.cmd.crc = SendBuf.crc();

//	while (serial->available())
//		serial->read();
	//Serial.print("s");
	//Serial.println(cmd, HEX);

	serial->write(bytes, sizeof(SendBuf));
}

