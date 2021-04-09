/*
 * taskCommunication.c
 *
 *  Created on: 4 gru 2020
 *      Author: Karol
 */

#include <task_bilge_pump.h>
#include "cmsis_os.h"
#include "usart.h"
#include "taskCommunication.h"
#include "rbx_decoder.h"
#include "uart_interrupt.h"

#define SIZE 9
#include <string.h>
#define RX_SIZE 255
//decoded chars
uint8_t ParseBuffer[RX_SIZE];

osMailQDef(mail, 16, SBT_s_pump_state_input);                    // Define mail queue
osMailQId  mail;

uint32_t rxPrt = 0;
volatile bool rxFull = false;
uint8_t rawData[99];
uint8_t rawDataPtr=0;
extern volatile SBT_e_pump_mode g_pump_mode;

void uartRxClb(uint8_t data)
{
	decoderPut(data);
	rawData[rawDataPtr]=data;
	rawDataPtr= (rawDataPtr+1)%99;
}

void writeBytes(uint8_t *data, uint16_t len)
{
	uart_send_msg(data, len, 100);
}

void decoderData(uint8_t command, uint8_t dataLen, uint8_t dataPtr, uint8_t data, PACKET_STATUS_ET status)
{
//this run from isr and put one char at time
	ParseBuffer[dataPtr] = data;

	if (dataLen == dataPtr)
	{

		if (status == PACKET_OK)
		{
			if (command == PUMP_ID) //process start
			{
				SBT_s_pump_state_input *pumpPacket;
				pumpPacket = osMailAlloc(mail, osWaitForever);

				if( pumpPacket == NULL ) return;
				size_t bytes = sizeof(SBT_s_pump_state_input);
				if (dataLen != bytes)
					return;
				memcpy(pumpPacket, ParseBuffer, bytes);
				osMailPut(mail, pumpPacket);
			}
			if (command == PUMP_MODE_ID)
			{
				g_pump_mode = ParseBuffer[0];
			}

/*
			if (command == 10) //process start
			{
				size_t bytes = sizeof(processSetting);
				if (dataLen != bytes)
					return;
				memcpy(&processSetting, ParseBuffer, bytes);
				received++;
				receivedFlag++;
				addrToAck = command;
				processStart = true;
				processRampRestart = true;
			}
			*/
		}

	}
}

//osSemaphoreId  osSemDecoder;
//osSemaphoreDef(osSemDecoder);

extern osSemaphoreId decoder_semHandle;


//osSemaphoreDef(osSemDecoder);
//osSemaphoreId (osSemDecoder_id);

void decoderSendDataRtos(uint8_t command, uint8_t dataLen, uint8_t *data){
	osStatus status;
    //static osSemDecoder_id;
  //  if(osSemDecoder_id == NULL )

    osSemaphoreWait(decoder_semHandle, osWaitForever);
    decoderSendData( command,  dataLen, data);
    status = osSemaphoreRelease(decoder_semHandle);
    if(status != osOK){
    	status++;
    }
  }

UBaseType_t freeStack_default;
UBaseType_t freeStack_pidHandle;
UBaseType_t freeStack_temperatureHandle;
UBaseType_t freeStack_UARTHandle;
uint32_t refreshed = 0;




void StartTaskCommunication(void const * argument){


	mail = osMailCreate(osMailQ(mail), NULL);
	//osSemDecoder = osSemaphoreCreate(osSemaphore(osSemDecoder), 1);
	uart_interrupts_enable();

	while(1){
		uint32_t PreviousWakeTime = osKernelSysTick();

		osDelayUntil(&PreviousWakeTime, TASK_COMMUNICATION_DELAY);
	}
}







