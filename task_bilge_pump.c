#include "task_bilge_pump.h"
#include "cmsis_os.h"

/*
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim2;
*/

volatile uint16_t bilge_parameters[2] = {0, 0}; // [0] -- pump current; [1] -- water sensor;

volatile SBT_e_bilge_mode g_bilge_mode = PUMP_AUTO; // Pump mode default set to PUMP_AUTO


extern osMailQId  mail;


int SBT_Bilge_Auto_Controll(SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_fatal_error->water_detected_current){
		return PUMP_AUTO_ON;
	}
	else{
		return PUMP_AUTO_OFF;
	}
}


void SBT_Bilge_State_Check(SBT_s_bilge_fatal_error* bilge_fatal_error){
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, SET);
		osDelay(MEASUREMENT_DELAY);

		// Obtain current

		// Check for water presence
		SBT_Bilge_Detect_Water_By_Current(bilge_fatal_error);
		// Check for pump errors
		SBT_Bilge_Pump_Error_Analysis(bilge_fatal_error);

		/*
		if(bilge_fatal_error->water_detected_current){
			water_detected_times++;
		}
		else{
			water_detected_times = 0;
		}
		loop_ctr = 0; */

	}



void SBT_Bilge_Detect_Water_By_Current(SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_parameters[Pump_Current] >= CURRENT_WATER_DETECTED){
		bilge_fatal_error->water_detected_current = 1;
		SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_CURRENT);
		return;
	}
	else{
		bilge_fatal_error->water_detected_current = 0;
		return;
	}
}

void SBT_Bilge_Detect_Water_By_Sensor(SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_parameters[Water_Sensor]){
		bilge_fatal_error->water_detected_sensor = 1;
		SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_SENSOR);
		return;
	}
	else{
		bilge_fatal_error->water_detected_sensor = 0;
		return;
	}
}


void SBT_Bilge_Pump_Error_Analysis(SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_parameters[Pump_Current] <= CURRENT_ZERO){
		bilge_fatal_error->bilge_current_zero = 1;
		SBT_Bilge_System_Failure(BILGE_CURRENT_ZERO);
		return;
	}
	else if(bilge_parameters[Pump_Current] >= CURRENT_OVER_CURRENT){
		bilge_fatal_error->bilge_overcurrent = 1;
		SBT_Bilge_System_Failure(BILGE_OVERCURRENT);
		return;
	}
	else if(bilge_parameters[Pump_Current] < 0){ // FIX ME
		bilge_fatal_error->current_read_error = 1;
		SBT_Bilge_System_Failure(CURRENT_READ_ERROR);
		return;
	}
	else {
		bilge_fatal_error->bilge_current_zero = 0;
		bilge_fatal_error->bilge_overcurrent = 0;
		return;
	}
}


void SBT_Water_Sensor_Error_Analysis(SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_parameters[Water_Sensor] == 0){ // FIX ME
		bilge_fatal_error->water_sensor_read_error = 1;
		SBT_Bilge_System_Failure(WATER_SENSOR_READ_ERROR);
		return;
	}
	else{
		bilge_fatal_error->water_sensor_read_error = 0;
		return;
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
			msg[0]= BILGE_WATER_DETECTED_BY_SENSOR;
			break;
		}

		case BILGE_WATER_DETECTED_BY_CURRENT_FOR_TOO_LONG:{
			msg[0]= BILGE_WATER_DETECTED_BY_CURRENT_FOR_TOO_LONG;
			break;
		}

		case WATER_SENSOR_READ_ERROR:{
			msg[0]= WATER_SENSOR_READ_ERROR;
			break;
		}

		case CURRENT_READ_ERROR:{
			msg[0]= CURRENT_READ_ERROR;
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
	BILGE_FATAL_ERROR_INIT(fatal_error);
	SBT_s_bilge_fatal_error *bilge_fatal_error = &fatal_error;

	// Set variables values on init
	SBT_e_bilge_auto_action bilge_auto_action = PUMP_AUTO_OFF;

	uint8_t water_detected_times = 0;
	uint8_t loop_ctr = 0;
	osEvent evt;
//delay 3s for boat startup
//pump seft test for zero current/, over current and short current
	while(1){
		// Connection timeout check
		if(evt.status == osEventMail){
			if(bilge_auto_action == PUMP_AUTO_TIMEOUT){
				g_bilge_mode = PUMP_AUTO; // Turn on Pump Auto Mode when return from timeout
			}

			if(g_bilge_mode != PUMP_OFF){
//				 if(loop_ctr >= MEASUREMENT_PERIOD && g_bilge_mode != PUMP_OFF){ // What about measure during error?

				SBT_Bilge_State_Check(bilge_fatal_error);
				if(water_detected_times >= WATER_DETECTED_CRITICAL_TIME){
					SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_CURRENT_FOR_TOO_LONG);
				}
			}
		}
		else{
			SBT_Bilge_System_Failure(TIMEOUT);
			bilge_auto_action = PUMP_AUTO_TIMEOUT; // Variable to check when return from timeout

			if(bilge_fatal_error->bilge_current_zero || bilge_fatal_error->bilge_overcurrent){
				g_bilge_mode = PUMP_OFF; // Cannot turn on pump when there is no info about end of error
			}
			else{
				g_bilge_mode = PUMP_ON;

//				if(loop_ctr >= MEASUREMENT_PERIOD && g_bilge_mode != PUMP_OFF){ // What about measure during error?

				SBT_Bilge_State_Check(bilge_fatal_error);
				if(water_detected_times >= WATER_DETECTED_CRITICAL_TIME){
					SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_CURRENT_FOR_TOO_LONG);
				}
			}


		// Water sensor handling
		// Check for water presence by sensor
		SBT_Bilge_Detect_Water_By_Sensor(bilge_fatal_error);
		// Check for water sensor errors
		SBT_Water_Sensor_Error_Analysis(bilge_fatal_error);



		// Pump Mode handling
		switch(g_bilge_mode){
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
				if(bilge_fatal_error->bilge_current_zero){
					HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);

					// change pump check time
					break;
				}
				else if (bilge_fatal_error->bilge_overcurrent){
					HAL_GPIO_WritePin(Cooling_Pump_GPIO_Port, Cooling_Pump_Pin, RESET);

					// change pump check time
					break;
				}

				bilge_auto_action = SBT_Bilge_Auto_Controll(bilge_fatal_error);

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
