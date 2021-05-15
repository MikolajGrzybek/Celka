#include "task_bilge_pump.h"
#include "cmsis_os.h"
#include "task_can_rx.h"

// Pump mode default set to BILGE_PUMP_AUTO
volatile SBT_e_bilge_mode g_bilge_mode = BILGE_PUMP_AUTO;

// DMA variables for ADC1 measurement
uint16_t adc[100], buffer[100];


// ------------------------------------------- PUMP RELATED -------------------------------------------

int SBT_Bilge_Auto_Controll(SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_fatal_error->water_detected_current){
		return BILGE_PUMP_AUTO_ON;
	}
	else{
		return BILGE_PUMP_AUTO_OFF;
	}
}


void SBT_Bilge_State_Check(uint16_t delay, SBT_s_bilge_fatal_error* bilge_fatal_error) {
	uint16_t srednia = 0;
	uint16_t buffer_sum = 0;
	uint16_t bilge_current = 0;

	HAL_GPIO_WritePin(BIG_PUMP_GPIO_Port, BIG_PUMP_Pin, SET);
	delayMs(delay);

	// Calculate avarge current value
	for(int i = 1; i < 100; i++){ // Pump's current is in array index
		if((i-1) % 3 == 0){
			buffer_sum += buffer[i];
		}
	}

	srednia = buffer_sum / 33;

	// ADC value conversion to current [mA]
	bilge_current = srednia * V_REF / ADC_RANGE / KU * SHUNT_RESISTANCE; // check it ||nieodwracajacy op-amp||

	// Check for water presence
	SBT_Bilge_Detect_Water_By_Current(bilge_current, bilge_fatal_error);
	// Check for pump errors
	SBT_Bilge_Pump_Error_Analysis(bilge_current, bilge_fatal_error);
	return;
	}


void SBT_Bilge_Detect_Water_By_Current(uint16_t bilge_current, SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_current >= CURRENT_WATER_DETECTED && bilge_current < CURRENT_OVERCURRENT){
		bilge_fatal_error->water_detected_current = 1;
		SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_CURRENT);
		return;
	}
	else{
		bilge_fatal_error->water_detected_current = 0;
		return;
	}
}


void SBT_Bilge_Pump_Error_Analysis(uint16_t bilge_current, SBT_s_bilge_fatal_error* bilge_fatal_error){
	if(bilge_current <= CURRENT_ZERO){
		bilge_fatal_error->bilge_current_zero = 1;
		SBT_Bilge_System_Failure(BILGE_CURRENT_ZERO);
		return;
	}
	else if(bilge_current >= CURRENT_OVERCURRENT){
		bilge_fatal_error->bilge_overcurrent = 1;
		SBT_Bilge_System_Failure(BILGE_OVERCURRENT);
		return;
	}
	else if(bilge_current < 0){ // FIX ME
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


// -------------------------------------- WARER SENSOR RELATED ----------------------------------------

void SBT_Bilge_Water_Sensor(SBT_s_bilge_fatal_error* bilge_fatal_error){
	uint8_t water_detected = 0;
	water_detected = HAL_GPIO_ReadPin(GPIO_WATER_SENSOR_GPIO_Port, GPIO_WATER_SENSOR_Pin);
	if(water_detected){
		bilge_fatal_error->water_detected_sensor = 1;
		SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_SENSOR);
		return;
	}
	else{
		bilge_fatal_error->water_detected_sensor = 0;
		return;
	}
}


// ----------------------------------------- SYSTEM RELATED -------------------------------------------

void SBT_Bilge_System_Failure(SBT_e_bilge_event bilge_event_id){
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

		case CURRENT_READ_ERROR:{
			msg[0]= CURRENT_READ_ERROR;
			break;
		}

		case BILGE_AUTO_MODE_OFF:{
			msg[0]= BILGE_AUTO_MODE_OFF;
			break;
		}

		case BILGE_TIMEOUT:{
			msg[0]= TIMEOUT;
            break;
		}

		default:
			return;


	}
	SBT_Can_Send(BILGE_SYSTEM_ERRORS_ID, msg, 1000);
}


void StartTaskBilgePump(void const * argument){
	// Init struct values with 0
	BILGE_FATAL_ERROR_INIT(fatal_error);
	SBT_s_bilge_fatal_error *bilge_fatal_error = &fatal_error;

	// Set variables values on init
	SBT_e_bilge_auto_action bilge_auto_action = BILGE_PUMP_AUTO_OFF;
	uint32_t reportTimeCh1 = 0;
	uint32_t measure_time = MEASUREMENT_DELAY;
	uint8_t water_detected_times = 0;
	uint8_t pump_fatal_errors = 0;
	uint8_t pump_dead = 0;

	// Turn on DMA for pump's current 
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buffer, 100);

	// Start up self check
	SBT_Bilge_State_Check(START_UP_DELAY, bilge_fatal_error);
	HAL_GPIO_WritePin(BILGE_PUMP_Port, BILGE_PUMP_Pin, RESET);

	while(1){
		// ----------------------- PROCESS VARIABLES RELATED -----------------------

		if(g_bilge_mode != BILGE_PUMP_OFF){
			// Wait for given measure_time
			if(timerGetRemaining(reportTimeCh1, measure_time) <= 0){
				if(!pump_dead){
					SBT_Bilge_State_Check(PEAK_DELAY, bilge_fatal_error);

					// Serious leak detection
					if(bilge_fatal_error->water_detected_current){
						water_detected_times ++;
					}

					// Repeated overcurrent error detection
					if(bilge_fatal_error->bilge_overcurrent){
						bilge_auto_action = BILGE_PUMP_AUTO_OFF;
						pump_fatal_errors ++;
					}
					else{
						pump_fatal_errors = 0;
					}
				}
				timerArm(&reportTimeCh1);
			}
		}

		// Water sensor handling
		SBT_Bilge_Water_Sensor(bilge_fatal_error);


		// ----------------------------- MODE RELATED -----------------------------

		switch(g_bilge_mode){
			case BILGE_PUMP_ON:{
				HAL_GPIO_WritePin(BIG_PUMP_GPIO_Port, BIG_PUMP_Pin, SET);
				break;
			}

			case BILGE_PUMP_OFF:{
				HAL_GPIO_WritePin(BIG_PUMP_GPIO_Port, BIG_PUMP_Pin, RESET);
				break;
			}

			case BILGE_PUMP_AUTO:{
				bilge_auto_action = SBT_Bilge_Auto_Controll(bilge_fatal_error);


				// Fatal error handling
				if(pump_fatal_errors == 1){
					measure_time = MEASUREMENT_DELAY_CHECK_FATAL; // Next measure in 10 [min]
				}
				else if(pump_fatal_errors >= 2){
					pump_fatal_errors = 2;
					bilge_auto_action = BILGE_PUMP_AUTO_OFF;
					SBT_Bilge_System_Failure(BILGE_AUTO_MODE_OFF);
					pump_dead = 1; // Bilge pump will no longer turn on in auto
				}
				else{
					measure_time = MEASUREMENT_DELAY; // Back to deafult measure time when no overcurrent 
				}


				switch(bilge_auto_action){
					case BILGE_PUMP_AUTO_ON:{
						HAL_GPIO_WritePin(BIG_PUMP_GPIO_Port, BIG_PUMP_Pin, SET);

						// Make sure of water presence
						if(water_detected_times == 1){
							measure_time = MEASUREMENT_DELAY_CHECK_WATER_FIRST;
						}
						else if(water_detected_times >= 2){
							water_detected_times = 2;
							SBT_Bilge_System_Failure(BILGE_WATER_DETECTED_BY_CURRENT_FOR_TOO_LONG);
							measure_time = MEASUREMENT_DELAY_CHECK_WATER_SECOND;
						}
						else{
							measure_time = MEASUREMENT_DELAY;
						}
						break;
					}
					case BILGE_PUMP_AUTO_OFF:{
						HAL_GPIO_WritePin(BIG_PUMP_GPIO_Port, BIG_PUMP_Pin, RESET);

						water_detected_times = 0;
						break;
					}
				} 
				break;
				}
			}
		osDelay(100);
		}
	}
