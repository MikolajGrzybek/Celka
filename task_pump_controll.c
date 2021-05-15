#include "task_pump_controll.h"
#include "cmsis_os.h"
#include "sbt_can_msg.h"
#include "sbt_queues_and_semaphores.h"
#include "task_can_rx.h"

// Pump mode default set to AUTO_MODE
volatile SBT_e_pump_mode g_pump_mode = 2;  


int SBT_Pump_Controll(int16_t process_temperature, uint8_t pump_pin_state, uint8_t stop_temp, uint8_t start_temp){
	// Hysteresis control
	if (process_temperature <= stop_temp && pump_pin_state == 1){
		return 0; //Pump auto stop
	}
	else if(process_temperature >= start_temp && pump_pin_state == 0){
		return 1; //Pump auto start
	}

	return 0;
}


int SBT_Designate_Process_Temperature(can_pump_state_input* p_pump_state_input){
	if(p_pump_state_input->motor_one_temp > p_pump_state_input->motor_two_temp){
		return p_pump_state_input->motor_one_temp;
	}
	else {
		return p_pump_state_input->motor_two_temp;
	}
}


int SBT_Temperature_Analysis(can_pump_state_input* p_pump_state_input, int16_t process_temperature, uint8_t pump_pin_state){
	if(process_temperature >= MAX_OVER_TEMP) {
		return OVERHEAT;
	}
	return NONE;
}



int SBT_Flow_Analysis(int16_t input_flow, int16_t output_flow, uint8_t pump_pin_state){
	if(input_flow < MIN_INPUT_FLOW) {
		return DRY_RUNNING;
	}
	else if(input_flow * output_flow < 0) { // value of one flow meter is -1 -> broken pipes
		return LEAK;
	}
	else {
		return NONE;
	}
}


void SBT_System_Failure(SBT_e_pump_alarm error_id){
	// SEND TO CAN
	// It gets flag about system's condition and fatal errors
	uint8_t msg[8];
	switch(error_id){
		case DRY_RUNNING:{
			msg[0]= DRY_RUNNING;
			break;
		}

		case LEAK:{
			msg[0]= LEAK;
			break;
		}

		case OVERHEAT:{
			msg[0]= OVERHEAT;
		    break;
		}

		case TIMEOUT:{
			msg[0]= TIMEOUT;
            break;
		}
		default:
			return;


	}
	SBT_Can_Send(COOLING_SYSTEM_ERRORS_ID, msg, 1000);
}


void StartTaskPumpControll(void const * argument){
	PUMP_STATE_INPUT_INIT(pump_msg);
	can_pump_state_input *p_msg_received = &pump_msg;
	SBT_s_pump_fatal_alarm pump_fatal_alarm = {.PUMP_STOP = 0, .LEAK= 0, .TEMP_READ_ERROR = 0, .OVERHEAT = 0};
	SBT_e_pump_alarm p_pump_alarm = NONE; //
	SBT_e_pump_auto_action pump_auto_action = PUMP_AUTO_OFF; // Turn pump off deafultly on automatic mode
	uint8_t pump_pin_state = 0;	// Init as pump turned off as it is checked and used later
	int16_t process_temperature = 0; // Temperature handed to control function
	uint8_t loop_ctr = 0; // Counter for flow analysis purpose
	osEvent evt;
 
	while(1){
		// Get data from queue
		evt = osMailGet(can_cooling_parameters, TASK_PUMP_CONTROLL_COMMUNICATION_DELAY);

		if(evt.status == osEventMail){
			p_msg_received = evt.value.p; // Process values update when available
		}

		// Connection timeout check
		else{
			SBT_System_Failure(TIMEOUT);
			if(pump_fatal_alarm.PUMP_STOP || pump_fatal_alarm.LEAK){
				pump_auto_action = PUMP_AUTO_OFF;
				SBT_System_Failure(TIMEOUT); // todo: add no longer turn on in enum
			}
			else{
				pump_auto_action = PUMP_AUTO_ON;
			}
		}

		// Check Cooling_Pump_Pin output state
		pump_pin_state = HAL_GPIO_ReadPin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin);

		// Deisgnate process temperature
		process_temperature = SBT_Designate_Process_Temperature(p_msg_received);

		// Temperature analysis
		if (p_pump_alarm == OVERHEAT){
			pump_fatal_alarm.OVERHEAT = 1;
			SBT_System_Failure(OVERHEAT);
		}
		else if (p_pump_alarm == NONE){
			pump_fatal_alarm.TEMP_READ_ERROR = 0;
			pump_fatal_alarm.OVERHEAT = 0;
		}


		// Flow analysis. Rougly every 20 seconds
		if(loop_ctr >= 100){
			p_pump_alarm = SBT_Flow_Analysis(p_msg_received->input_flow, p_msg_received->output_flow, pump_pin_state);
			// Fatal error msg handling (just for manual control purpose)
 			if(p_pump_alarm == DRY_RUNNING){
				if(pump_pin_state){
					SBT_System_Failure(DRY_RUNNING);
					pump_fatal_alarm.PUMP_STOP = 1;
				}
				loop_ctr = 0;
			}
			else if(p_pump_alarm == LEAK){
				pump_fatal_alarm.LEAK = 1;
				SBT_System_Failure(LEAK);
				loop_ctr = 0;
			}
			else{
				pump_fatal_alarm.PUMP_STOP = 0;
				pump_fatal_alarm.LEAK = 0;
				loop_ctr = 0;
			}
		}

		// Pump Mode handling
		switch(g_pump_mode){
			case PUMP_ON:{
				HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, SET);
				break;
			}

			case PUMP_OFF:{
				HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);
				break;
			}	

			case PUMP_AUTO:{	
				// Fatal error handling
				if(pump_fatal_alarm.PUMP_STOP){
					HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);
					break;
				}
				else if(pump_fatal_alarm.LEAK){
					HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);
					break;
				}
				// Regular controll
				pump_auto_action = SBT_Pump_Controll(process_temperature, pump_pin_state, STOP_PUMP_TEMP, START_PUMP_TEMP);

				switch(pump_auto_action){
					case PUMP_AUTO_ON:{
						HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, SET);
						break;
					}
					case PUMP_AUTO_OFF:{
						HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);
						break;
					}
				}

				if(pump_fatal_alarm.OVERHEAT){
					SBT_System_Failure(OVERHEAT);
				}
				break;
			}	
		}
		loop_ctr++;
		osMailFree(can_cooling_parameters, p_msg_received);
		osDelay(100);
	}
}
