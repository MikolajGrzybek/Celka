#ifndef INC_TASKPUMPCONTROLL_H_
#define INC_TASKPUMPCONTROLL_H_

#include "stdbool.h"
#include "main.h"


// Struct values init macro
#define PUMP_STATE_INPUT_INIT(X) SBT_s_pump_state_input X = {.bilge_current = 0}
// Struct pump fatal alarms init
#define PUMP_FATAL_ALARM_INIT(X) SBT_s_pump_fatal_alarm X = {.pump_current_zero = 0, .pump_overcurrent = 0, .pump_water_detected = 0}
// Bilge pump motor's current in [z]
#define CURRENT_ZERO 0
#define CURRENT_NO_WATER 100	
#define CURRENT_WATER_SPLASH 500
#define CURRENT_WATER_DETECTED 700	
#define CURRENT_OVER_CURRENT 1500
#define TASK_BILGE_PUMP_COMMUNICATION_DELAY 1000
#define Cooling_Pump_GPIO_Port LD2_GPIO_Port
#define Cooling_Pump_Pin LD2_Pin



typedef struct {
    int16_t motor_one_temp; 
    int16_t motor_two_temp;
    int16_t bilge_current;		// Tylko to, reszte jebać
    int16_t output_flow;
} SBT_s_pump_state_input;

typedef enum {
	PUMP_BROKEN,
	PUMP_OVERCURRENT,
	WATER_DETECTED,
	NONE,
	TIMEOUT
} SBT_e_pump_alarm;

typedef struct {
	uint8_t pump_current_zero;
	uint8_t pump_overcurrent;
	uint8_t pump_water_detected;
} SBT_s_pump_fatal_alarm;

typedef enum {
	ZERO,
	NO_WATER,
	WATER_SPLASH_DETECTED,
	WATER_DETECTED,
	OVERCURRENT
} SBT_e_bilge_current_state;

typedef enum {
	PUMP_ON,
	PUMP_OFF,
	PUMP_AUTO
} SBT_e_pump_mode;

typedef enum{
	PUMP_AUTO_OFF,
	PUMP_AUTO_ON,
	PUMP_AUTO_TIMEOUT
} SBT_e_pump_auto_action;



/**
 * @brief Main controll function
 * 
 * @param SBT_e_bilge_current_state - dermine which algotithm apply 
 * 
 * @retval ID of SBT_e_pump_auto_action 
 *
 */
int SBT_Pump_Controll(SBT_e_bilge_current_state current_state);


/**
 * @brief Analysis of bilge pump current
 * 
 * @param bilge_current - measure behind pump
 *
 * @retval ID of detected state to SBT_e_bilge_current_state
 *
 */
int SBT_Bilge_Current_Analysis(int16_t bilge_current);


/**
 * @brief Sents error messeage to CAN queue
 *
 * @param error_id 
 *
 * @retval None
 *
 */
void SBT_System_Failure(SBT_e_pump_alarm error_id);


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

