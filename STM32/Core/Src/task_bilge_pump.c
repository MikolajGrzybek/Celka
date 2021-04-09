#include "task_bilge_pump.h"
#include "cmsis_os.h"


// Pump mode default set to AUTO_MODE
volatile SBT_e_pump_mode g_pump_mode = 2;  
extern osMailQId  mail;


int SBT_Pump_Controll(SBT_e_bilge_current_state current_state){
	return 0;
}


int SBT_Bilge_Pump_Current_Analysis(int16_t bilge_current){
	if(bilge_current <= CURRENT_ZERO){
		return 0; // Current = 0
	}
	else if(bilge_current >= CURRENT_NO_WATER && bilge_current < CURRENT_WATER_DETECTED){
		return 1; // Current = no water
	}
	else if(bilge_current >= CURRENT_WATER_SPLASH && bilge_current < CURRENT_WATER_DETECTED){
		return 2; // Current = splash of water detected
	}
	else if(bilge_current >= CURRENT_WATER_DETECTED && bilge_current < CURRENT_OVER_CURRENT){
		return 3; // Current = plenty of water detected
	}
	else if(bilge_current >= CURRENT_OVER_CURRENT){
		return 4; // Current = Overcurrent
	}
	
}


void SBT_System_Failure(SBT_e_pump_alarm error_id){
	// SEND TO CAN
	// It gets flag about system's condition and fatal errors
	uint8_t msg[8];
	switch(error_id){
		case PUMP_BROKEN:{
			msg[0]= PUMP_BROKEN;
			break;
		}

		case PUMP_OVERCURRENT:{
			msg[0]= PUMP_OVERCURRENT;
            break;
		}

		case TIMEOUT:{
			msg[0]= TIMEOUT;
            break;
		}
		default:
			return;


	}
	// SBT_Can_Send(COOLING_SYSTEM_ERRORS, msg, 1000);
}


void Start_Task_PumpControl(void const * argument){
	PUMP_STATE_INPUT_INIT(pump_msg);
	SBT_s_pump_state_input *p_msg_received = &pump_msg;
	PUMP_FATAL_ALARM_INIT(pump_fatal_alarm);
	SBT_e_bilge_current_state current_state = ZERO; // Set current to 0 on init
	SBT_e_pump_auto_action pump_auto_action = PUMP_AUTO_OFF; // Turn pump off deafultly on automatic mode
	uint8_t loop_ctr = 0; // Counter for current analysis purpose
	osEvent evt;


	while(1){
		// Get data from queue
		evt = osMailGet(mail, TASK_BILGE_PUMP_COMMUNICATION_DELAY);

		if(evt.status == osEventMail){
			p_msg_received = evt.value.p; // Process values update when available
			if(pump_auto_action == PUMP_AUTO_TIMEOUT){
				g_pump_mode = PUMP_AUTO; // Turn on Pump Auto Mode when return from timeout
			}
		}

		// Connection timeout check
		else{
			SBT_System_Failure(TIMEOUT);
			pump_auto_action = PUMP_AUTO_TIMEOUT; // Variable to check when return from timeout
			if(pump_fatal_alarm.pump_current_zero || pump_fatal_alarm.pump_overcurrent){
				g_pump_mode = PUMP_OFF;
			}
			else{
				g_pump_mode = PUMP_ON;
			}
		}


		// Flow analysis. Roughly every 30 seconds
		if(loop_ctr >= 30 && g_pump_mode != PUMP_OFF){
			// Read pump's current
			HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, SET);
			osDelay(MEASURMENT_TIME);
			// Obtain pumps current value here
			current_state = SBT_Bilge_Pump_Current_Analysis(p_msg_received->bilge_current);
			if(pump_auto_action != PUMP_AUTO_ON){
				HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, RESET);
			}

			// Fatal error msg handling (just for manual control purpose)
			if(current_state == ZERO){
				pump_fatal_alarm.pump_current_zero = 1;
				SBT_System_Failure(PUMP_BROKEN);
				loop_ctr = 0;
			}
			else if(current_state == OVERCURRENT){
				pump_fatal_alarm.pump_overcurrent = 1;
				SBT_System_Failure(PUMP_OVERCURRENT);
				loop_ctr = 0;
			}
			else if(current_state == WATER_DETECTED){
				pump_fatal_alarm.pump_water_detected = 1;
				SBT_System_Failure(PUMP_WATER_DETECTED);
				loop_ctr = 0;
			}
			else{
				pump_fatal_alarm.pump_current_zero = 0;
				pump_fatal_alarm.pump_overcurrent = 0;
				loop_ctr = 0;
			}
		}

		// Pump Mode handling
		switch(g_pump_mode){
			case PUMP_ON:{
				HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, SET);
				
				// chceck current
				
				break;
			}

			case PUMP_OFF:{
				HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);

				// chceck current

				break;
			}

			case PUMP_AUTO:{
				// Fatal error handling
				if(pump_fatal_alarm.pump_current_zero || pump_fatal_alarm.pump_overcurrent){
					HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);
					break;
				}

				// Regular controll
				pump_auto_action = SBT_Pump_Controll(current_state);

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
				break;
			}
		}
		loop_ctr++;
		osMailFree(mail, p_msg_received);
	}
}
