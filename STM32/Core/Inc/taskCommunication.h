/*
 * taskCommunication.h
 *
 *  Created on: 4 gru 2020
 *      Author: Karol
 */

#ifndef INC_TASKCOMMUNICATION_H_
#define INC_TASKCOMMUNICATION_H_

#define TASK_COMMUNICATION_DELAY 1000 // task delay in ms

#include "uart_control_msg.h"

void StartTaskCommunication(void const * argument);
void decoderSendDataRtos(uint8_t command, uint8_t dataLen, uint8_t *data);



#endif /* INC_TASKCOMMUNICATION_H_ */
