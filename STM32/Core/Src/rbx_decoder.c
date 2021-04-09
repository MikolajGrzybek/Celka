/*
 * rbx_decoder.c
 *
 *  Created on: 8 gru 2020
 *      Author: Karol
 */


#include "rbx_decoder.h"
//#include <stdio.h>
#include <string.h>

uint8_t packetBuff[64] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

__weak void writeBytes(uint8_t *data, uint16_t len) {
	while (len--) {
		//packetBuff[packet_i++] = *data++;
    	//uart_put_char here
	}
}

__weak void decoderText(uint8_t chr) {
	//override this function to get text unknown by decoder char by char
	// print char
	volatile static uint8_t j=0;
	j++;
}

__weak void decoderData(uint8_t command, uint8_t dataLen, uint8_t dataPtr,
		uint8_t data, PACKET_STATUS_ET status) {
	//override this function to get data byte by byte
	volatile static uint8_t i=0;
	i++;
}

//https://en.wikipedia.org/wiki/Fletcher%27s_checksum
volatile uint32_t c0, c1;
void crcReset() {
	c0 = 0x12; //just random small nonzero seed value
	c1 = 0x34;
}

void crcCompute(uint8_t data) {
	//fletcher16(const uint8_t *data, size_t len) from wikipedia
	//this work up to 5000 bytes. Above just call somewhere crcGet to calculate modulo.
	//Detalis here: https://en.wikipedia.org/wiki/Fletcher%27s_checksum
	c0 = c0 + data;
	c1 = c1 + c0;
}

void crcGet(uint8_t *crc, uint8_t len) {
	while (len < 2) {
		len++;//configuration error
	}
	c0 = c0 % 255;
	c1 = c1 % 255;
	crc[0] = c0;
	crc[1] = c1;
}

bool crctest(uint8_t *crc, uint8_t len) {
	uint8_t crcComputed[CRC_SIZE];
	crcGet(crcComputed, len);
	if (memcmp(crc, crcComputed, len) == 0) {
		return true;
	} else {
		return false;
	}
}
////////////////////////  c file

const uint8_t START_OF_PACKET[] = { 0xC0, 0xFE }; //162, 252 dec, above asci table characters
const uint8_t END_OF_PACKET = 0xED;             // just EnD
volatile uint32_t guard1[128];
static DECODER_STATE_ST m_decoderNextState = INIT;
volatile uint32_t guard2[128];
/*
 uint16_t simpleHash16(uint8_t data, uint16_t crc)
 {
 uint32_t base = 1811; //prime number
 uint32_t result = base * crc + data + base;
 // in python result =result % (0xffffffff+1)
 result %= 52631; //52631;//big prime number
 return result & 0xffff;
 }
 */
uint8_t simpleHash8(uint8_t data, uint8_t crc) {
	uint32_t hash = crc ^ (data + 0b01010001);
	//return hash;
	hash *= 2731;
	//hash = hash + crc + data;
	hash %= 251;
	return hash;
}

void packetCRC(uint8_t *data, uint8_t dataLen, uint8_t command, uint8_t *crcTab,
		uint8_t crcLen) {
	uint8_t singleByte;
	crcReset();
	crcCompute(command);
	crcCompute(dataLen);
	while (dataLen--) {
		singleByte = *data;
		data++;
		crcCompute(singleByte);
	}
	crcGet(crcTab, crcLen);
}

void decoderSendData(uint8_t command, uint8_t dataLen, uint8_t *data) {
	uint8_t crcArray[CRC_SIZE];
	packetCRC(data, dataLen, command, crcArray, CRC_SIZE);

	writeBytes((uint8_t*) START_OF_PACKET, sizeof(START_OF_PACKET));
	writeBytes(&command, 1);
	writeBytes(&dataLen, 1);
	writeBytes(crcArray, CRC_SIZE);
	writeBytes(data, dataLen);
	writeBytes((uint8_t*) &END_OF_PACKET, 1);
}

DECODER_STATE_ST decoderGetState() {
	return m_decoderNextState;
}

void decoderReset() {
	m_decoderNextState = INIT;
}

volatile uint32_t decoded = 0;

void decoderPut(uint8_t chr) {
	static uint8_t crcReceived[CRC_SIZE];
	static uint8_t headerArr[sizeof(START_OF_PACKET)];
	static uint8_t command;
	static uint8_t dataLen;
	static uint8_t dataPtr;
	const size_t headerSize = sizeof(START_OF_PACKET) - 1;

	switch (m_decoderNextState) {
	case INIT:
		headerArr[0] = 0;
		dataPtr = 0;
		crcReceived[0] = 1;
		crcReceived[1] = 2;

		//go to header
	case HEADER:
		if (dataPtr < sizeof(START_OF_PACKET)) {
			dataPtr++; // wait for fill buffer, print only if buff is full and not match start sequence
		} else {
			decoderText(headerArr[0]);
		}
		memmove(&headerArr[0], &headerArr[1], headerSize); //shift left by 1 byte
		headerArr[sizeof(START_OF_PACKET) - 1] = chr;
		if (memcmp(headerArr, START_OF_PACKET, sizeof(START_OF_PACKET)) == 0) {
			m_decoderNextState = COMMAND;
		} else {
			m_decoderNextState = HEADER;
		}
		break;

	case COMMAND:
		decoded++;
		command = chr;
		m_decoderNextState = DATA_LEN;
		break;

	case DATA_LEN:
		dataLen = chr;
		m_decoderNextState = CRC_STATE;
		dataPtr = 0;
		break;

	case CRC_STATE:
		crcReceived[dataPtr] = chr;
		dataPtr++;
		m_decoderNextState = CRC_STATE;
		if (dataPtr >= CRC_SIZE) {
			crcReset();
			crcCompute(command);
			crcCompute(dataLen);
			dataPtr = 0;
			m_decoderNextState = DATA_FIELD;
		}
		break;

	case DATA_FIELD:
		if (dataPtr < dataLen -1) {
			m_decoderNextState = DATA_FIELD;
		} else {
			m_decoderNextState = END0;
		}
		decoderData(command, dataLen, dataPtr, chr, PACKET_READING);
		crcCompute(chr);
		dataPtr++;
		break;

	case END0:
		if (chr == END_OF_PACKET) {
			if (crctest(crcReceived, CRC_SIZE)) {
				decoderData(command, dataLen, dataPtr, 0, PACKET_OK);
			} else {
				decoderData(command, dataLen, dataPtr, 0, PACKET_CRCFAIL);
			}
		} else {
			decoderData(command, dataLen, dataPtr, 0, PACKET_END_CRCFAIL);
		}
		m_decoderNextState = INIT;
		break;

	default:
		m_decoderNextState = INIT;
		break;
	}
}
