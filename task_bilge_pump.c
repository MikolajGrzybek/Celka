#include "task_bilge_pump.h"
#include "cmsis_os.h"

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim2;

volatile SBT_e_bilge_mode g_bilge_mode = PUMP_AUTO; // Pump mode default set to PUMP_AUTO
extern osMailQId  mail;


int SBT_Bilge_Controll(SBT_s_bilge_fatal_error bilge_fatal_error){
	if(bilge_fatal_error.water_detected_current){
		return PUMP_AUTO_ON;
	}
	else{
		return PUMP_AUTO_OFF;
	}
}


void SBT_Bilge_Detect_Water_Current(uint16_t bilge_current, SBT_s_bilge_fatal_error bilge_fatal_error){
	if(bilge_current >= CURRENT_WATER_DETECTED){
		bilge_fatal_error.water_detected_current = 1;
		SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_CURRENT);
		return;
	}
	else{
		bilge_fatal_error.water_detected_current = 0;
		return;
	}
// FIX ME! What about current read error?
}


int SBT_Bilge_Error_Analysis(SBT_s_bilge_state_input* input_state, uint8_t bilge_pin_state){
	if(input_state->bilge_current <= CURRENT_ZERO && bilge_pin_state){
		return BILGE_CURRENT_ZERO;
	}
	else if(input_state->bilge_current >= CURRENT_OVER_CURRENT){
		return BILGE_OVERCURRENT;
	}
	else if(!input_state->water_sensor){ // FIX ME! What about water sensor read error?
		return WATER_SENSOR_READ_ERROR;
	}
	else{
		return NONE;
	}
}


void SBT_Bilge_Define_Error(SBT_e_bilge_event bilge_event_id, SBT_s_bilge_fatal_error bilge_fatal_error){

	if(bilge_event_id == BILGE_CURRENT_ZERO){
		bilge_fatal_error.bilge_current_zero = 1;
		SBT_Bilge_System_Failure(BILGE_CURRENT_ZERO);
	}
	else if(bilge_event_id == BILGE_OVERCURRENT){
		bilge_fatal_error.bilge_overcurrent = 1;
		SBT_Bilge_System_Failure(BILGE_OVERCURRENT);
	}
	else {
		bilge_fatal_error.bilge_current_zero = 0;
		bilge_fatal_error.bilge_overcurrent = 0;
	}
}


void SBT_Bilge_System_Failure(SBT_e_bilge_event bilge_event_id){
	// SEND TO CAN
	// It gets flag about system's condition and fatal errors
	uint8_t msg[8];
	switch(bilge_event_id){
		case BILGE_CURRENT_ZERO:{
			msg[0]= BILGE_CURRENT_ZERO;
			break;
		}

		case BILGE_OVERCURRENT:{
			msg[0]= BILGE_OVERCURRENT;
            break;
		}

		case BILGE_WATER_DETECTED_BY_CURRENT:{
			msg[0]= BILGE_WATER_DETECTED_BY_CURRENT;
			break;
		}

		case BILGE_WATER_DETECTED_BY_SENSOR:{
			msg[0]= BILGE_WATER_DETECTED_BY_CURRENT;
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
	// Set structs values on init
	BILGE_STATE_INPUT_INIT(bilge_msg);
	SBT_s_bilge_state_input *p_msg_received = &bilge_msg;
	BILGE_FATAL_ALARM_INIT(bilge_fatal_error);

	// Set variables values on init
	SBT_e_bilge_event bilge_event = NONE;
	SBT_e_bilge_auto_action bilge_auto_action = PUMP_AUTO_OFF;

	uint8_t bilge_pin_state = NONE;
	uint8_t loop_ctr = 0;
	osEvent evt;

	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); // WIP raczej nie tu

	while(1){
		// Get data from queue
		evt = osMailGet(mail, TASK_BILGE_PUMP_COMMUNICATION_DELAY);

		// Connection timeout check
		if(evt.status == osEventMail){
			p_msg_received = evt.value.p; // Process values update when available
			if(bilge_auto_action == PUMP_AUTO_TIMEOUT){
				g_bilge_mode = PUMP_AUTO; // Turn on Pump Auto Mode when return from timeout
			}
		}
		else{
			SBT_Bilge_System_Failure(TIMEOUT);
			bilge_auto_action = PUMP_AUTO_TIMEOUT; // Variable to check when return from timeout

			if(bilge_fatal_error.bilge_current_zero|| bilge_fatal_error.bilge_overcurrent){
				g_bilge_mode = PUMP_OFF; // Cannot turn on pump when there is no info about end of error
			}
			else{
				g_bilge_mode = PUMP_ON;
			}
		}


		// Can we check current again when bilge_current_zero || bilge_overcurrent ???

		// Pump's current analysis. Roughly every 30 seconds
		if(loop_ctr >= MEASUREMENT_PERIOD && g_bilge_mode != PUMP_OFF){
			HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, SET);
			osDelay(MEASUREMENT_DELAY);
			// Obtain current;

			// Check for bilge system error
			bilge_event = SBT_Bilge_Error_Analysis(p_msg_received, bilge_pin_state);
			// Handle bilge system error
			SBT_Bilge_Define_Error(bilge_event, bilge_fatal_error);
			// Check for water presence
			SBT_Bilge_Detect_Water_Current(p_msg_received->bilge_current, bilge_fatal_error);

			loop_ctr = 0;

		}

		// Get data from DMA
		HAL_ADC_Start_DMA(&hadc1, &p_msg_received->water_sensor, 1);

		bilge_event = SBT_Bilge_Error_Analysis(p_msg_received, bilge_pin_state); // again? same function 13 lines above

		if(bilge_event == WATER_SENSOR_READ_ERROR) {
			bilge_fatal_error.water_sensor_read_error = 1;
			SBT_Bilge_System_Failure(WATER_SENSOR_READ_ERROR);
		}
		else {
			bilge_fatal_error.water_sensor_read_error = 0;
		}


		// Read bilge pump pin state
		bilge_pin_state = HAL_GPIO_ReadPin(LD2_GPIO_Port, LD2_Pin);

		// Pump Mode handling
		switch(g_bilge_mode){
			case PUMP_ON:{
				HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, SET);
				
				break;
			}

			case PUMP_OFF:{
				HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);

				if(bilge_fatal_error.water_detected_sensor) {
					SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_SENSOR);
				}

				break;
			}

			case PUMP_AUTO:{
				// Fatal error handling
				if(bilge_fatal_error.bilge_current_zero){
					HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);
					break;
				}
				else if (bilge_fatal_error.bilge_overcurrent){
					HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);
					break;
				}

				//
				bilge_auto_action = SBT_Bilge_Controll(bilge_fatal_error);

				switch(bilge_auto_action){
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
