#ifndef INC_TASKPUMPCONTROLL_H_
#define INC_TASKPUMPCONTROLL_H_

#include "main.h"



// Struct values init macro
#define BILGE_STATE_INPUT_INIT(X) SBT_s_bilge_state_input X = {.bilge_current = 0, .water_sensor = 0}
// Struct pump fatal alarms init
#define BILGE_FATAL_ALARM_INIT(X) SBT_s_bilge_fatal_error X = {.bilge_current_zero = 0, .bilge_overcurrent = 0, .water_sensor_read_error = 0, .water_detected_current = 0, .water_detected_sensor = 0}


// Bilge pump motor's current in [mA]
#define CURRENT_ZERO 0
#define CURRENT_NO_WATER 100	
#define CURRENT_WATER_DETECTED 700	
#define CURRENT_OVER_CURRENT 1500
// Water sensor values
#define WATER_SENSOR_WATER_DETECTED	1000
// Current measurement settings in [ms]
#define MEASUREMENT_DELAY 200
#define MEASUREMENT_PERIOD 30
// Task Bilge Pump settings
#define TASK_BILGE_PUMP_COMMUNICATION_DELAY 1000 // [ms]

// Debug purpose
#define Cooling_Pump_GPIO_Port LD2_GPIO_Port
#define Cooling_Pump_Pin LD2_Pin



typedef struct {
    int16_t bilge_current; // Not sure of those types
	int16_t water_sensor;  // Not sure of those types
} SBT_s_bilge_state_input;

typedef enum {
	BILGE_WATER_DETECTED_BY_CURRENT,
	BILGE_WATER_DETECTED_BY_SENSOR,
	BILGE_CURRENT_ZERO,
	BILGE_OVERCURRENT,
	WATER_SENSOR_READ_ERROR,
	// CURRENT_READ_ERROR
	NONE,
	TIMEOUT
} SBT_e_bilge_event;

typedef struct {
	uint8_t bilge_current_zero;
	uint8_t bilge_overcurrent;
	uint8_t water_sensor_read_error;
	// uint8_t current_read_error;
	uint8_t water_detected_current;
	uint8_t water_detected_sensor;
} SBT_s_bilge_fatal_error;

typedef enum {
	PUMP_ON,
	PUMP_OFF,
	PUMP_AUTO
} SBT_e_bilge_mode;

typedef enum{
	PUMP_AUTO_OFF,
	PUMP_AUTO_ON,
	PUMP_AUTO_TIMEOUT
} SBT_e_bilge_auto_action;



/**
 * @brief Turns on pump in auto mode if water detected by current
 * 
 * @param bilge_fatal_error.water_detected_current
 * 
 * @retval ID of SBT_e_pump_auto_action
 * 
 */
int SBT_Bilge_Controll(SBT_s_bilge_fatal_error bilge_fatal_error);


/**
 * @brief Check for water by current value analysis.
 * 
 * @param bilge_current value
 *
 * @param bilge_fatal_error water_detected_current is set TRUE
 * if water detected
 *
 * @return None
 */
void SBT_Bilge_Detect_Water_Current(uint16_t bilge_current,  SBT_s_bilge_fatal_error bilge_fatal_error);


/**
 * @brief Analysis bilge pump state
 * 
 * @param input_state - current and water sensor values
 *
 * @param bilge_pin_state
 *
 * @retval ID of detected state
 *
 */
int SBT_Bilge_Error_Analysis(SBT_s_bilge_state_input* input_state, uint8_t bilge_pin_state);


/**
 * @brief If fatal error occurs, save it in SBT_s_bilge_fatal_error
 *
 * @param bilge_event_id
 *
 * @param bilge_fatal_error
 *
 * @retval None
 */
void SBT_Bilge_Define_Error(SBT_e_bilge_event bilge_event_id, SBT_s_bilge_fatal_error bilge_fatal_error);


/**
 * @brief Sents error messeage to CAN queue
 *
 * @param bilge_event_id
 *
 * @retval None
 *
 */
void SBT_Bilge_System_Failure(SBT_e_bilge_event bilge_event_id);


/**
 * @brief Main function of Task Pump Controller
 *
 * @param None
 *
 * @retval None
 *
 */
void Start_Task_PumpControl(void const * argument);

#endif /* INC_TASKPUMPCONTROLL_H_ */
