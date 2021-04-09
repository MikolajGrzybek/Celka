/*
 * rbx_decoder.h
 *
 *  Created on: 8 gru 2020
 *      Author: Karol
 */

#ifndef INC_RBX_DECODER_H_
#define INC_RBX_DECODER_H_

#include "main.h"
#include "stdbool.h"

typedef enum
{
    PACKET_READING,
    PACKET_OK,
    PACKET_CRCFAIL,
	PACKET_END_CRCFAIL
} PACKET_STATUS_ET;

//packet structure:
/*struct
{
    uint16_t START_VAL;
    uint8_t command;
    uint8_t dataLen;
    uint8_t crc;
    uint8_t data[255]; //dataLen, max 255
    uint8_t END_VAL;
} packet_example;
*/
typedef enum
{
    INIT,
    HEADER,
    COMMAND,
    DATA_LEN,
    CRC_STATE,
    DATA_FIELD,
    END0
} DECODER_STATE_ST;

#define CRC_SIZE 2
#define DECODER_ACK_ADR 255

void writeBytes(uint8_t *data, uint16_t len);
void decoderText(uint8_t chr);
void decoderData(uint8_t command, uint8_t dataLen, uint8_t dataPtr, uint8_t data, PACKET_STATUS_ET status);
void decoderSendData(uint8_t command, uint8_t dataLen, uint8_t *data);
DECODER_STATE_ST decoderGetState();
void decoderReset();
void decoderPut(uint8_t chr);




#endif /* INC_RBX_DECODER_H_ */
