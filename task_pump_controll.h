#ifndef INC_TASKPUMPCONTROLL_H_
#define INC_TASKPUMPCONTROLL_H_

#include "main.h"

// Macros to initiate structs vaules
#define PUMP_STATE_INPUT_INIT(X) SBT_s_pump_state_input X = {.motor_one_temp = 0, .motor_two_temp = 0, .input_flow = 0, .output_flow = 0}
#define PUMP_FATAL_ALARM_INIT(X) SBT_s_pump_fatal_alarm ={.PUMP_STOP = 0, .LEAK= 0, .TEMP_READ_ERROR = 0, .OVERHEAT = 0}

#define TASK_PUMP_CONTROLL_COMMUNICATION_DELAY 1000 // Dask delay in ms
#define START_PUMP_TEMP 50// Motor temperature in deg C
#define STOP_PUMP_TEMP 40 // Motor temperature in deg C
#define MIN_FLOW_DIFFERENCE 10  // Minimum flow difference
#define MIN_INPUT_FLOW 5 // Minimum flow difference
#define MAX_OVER_TEMP 100 // Top hysteresis overheated state value
#define MIN_OVER_TEMP 90 // Bottom hysteresis overheated state value
// just for debug
#define Cooling_Pump_GPIO_Port TIM8_CH2_ENGINE_PUMP_GPIO_Port
#define Cooling_Pump_Pin TIM8_CH2_ENGINE_PUMP_Pin

typedef struct {
    int16_t motor_one_temp;
    int16_t motor_two_temp;
    int16_t input_flow;
    int16_t output_flow;
} SBT_s_pump_state_input;

typedef enum {
	OVERHEAT,
	TEMPERATURE_READ_ERROR,
	DRY_RUNNING,
	LEAK,
	NONE,
	TIMEOUT
} SBT_e_pump_alarm;

typedef struct {
	uint8_t PUMP_STOP;
	uint8_t LEAK;
	uint8_t TEMP_READ_ERROR;
	uint8_t OVERHEAT;
} SBT_s_pump_fatal_alarm;

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
 * @brief Hysteresis controll
 *
 * @param process_temperature 
 * 
 * @param pump_pin_state 
 * 
 * @param stop_temp - turn off value
 *
 * @param start_temp - turn on value
 * 
 * @retval None
 *
 */
int SBT_Pump_Controll(int16_t process_temperature, uint8_t pump_pin_state, uint8_t stop_temp, uint8_t start_temp);

/**
 * @brief Designate procces temperature
 *
 * @param p_pump_state_input - pointer to pump state struct 
 *
 * @retval Value of process temperature
 *
 */
int SBT_Designate_Process_Temperature(SBT_s_pump_state_input* p_pump_state_input);


/**
 * @brief Analysis of motor temperature
 *
 * @param motor_one_temp
 *
 * @param motor_two_temp
 *
 * @param pump_pin_state
 *
 * @retval ID of detected error or NONE
 *
 */
int SBT_Temperature_Analysis(SBT_s_pump_state_input* p_pump_state_input, int16_t process_temperature, uint8_t pump_pin_state);


/**
 * @brief Analysis of water flow in cooling ciruit
 *
 * @param input_flow - measure behind pump
 *
 * @param output_flow - measure at the end of the cooling system
 *
 * @param pump_pin_state
 *
 * @retval ID of detected error or NONE
 *
 */
int SBT_Flow_Analysis(int16_t input_flow, int16_t output_flow, uint8_t pump_pin_state);


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
void StartTaskPumpControll(void const * argument);

#endif /* INC_TASKPUMPCONTROLL_H_ */

