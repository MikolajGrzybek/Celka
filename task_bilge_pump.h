#ifndef INC_TASKPUMPCONTROLL_H_
#define INC_TASKPUMPCONTROLL_H_

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "sbt_can_msg.h"


// Struct pump fatal alarms init
#define BILGE_FATAL_ERROR_INIT(X) SBT_s_bilge_fatal_error X = {.bilge_current_zero = 0, .bilge_overcurrent = 0, .water_detected_current = 0, .water_detected_sensor = 0}


// Bilge pump motor's current in [mA] unit
#define CURRENT_ZERO 0
#define CURRENT_NO_WATER 500
#define CURRENT_WATER_DETECTED 700
#define CURRENT_OVERCURRENT 1000
// Current measurement time settings
#define CURRENT_PEAK_DELAY 200 // 200  [ms]
#define MEASUREMENT_DELAY  5000 //now debug usually: 300000UL 5 [min]
#define MEASUREMENT_DELAY_CHECK_FATAL 10000 // 10 [min]
#define MEASUREMENT_DELAY_CHECK_WATER_FIRST 15000 // 10 [s]
#define MEASUREMENT_DELAY_CHECK_WATER_SECOND 20000 // 15 [s]
// Current pump's current array id
#define PUMP_CURRENT 99
// Current calculate consts
#define V_REF 3300 // [mV]
#define ADC_RANGE 4096 // [bits]
// Task Bilge Pump settings
#define TASK_BILGE_PUMP_COMMUNICATION_DELAY 1000 // 1 [s]


typedef enum {
	BILGE_WATER_DETECTED_BY_CURRENT,
	BILGE_WATER_DETECTED_BY_SENSOR,
	BILGE_WATER_DETECTED_BY_CURRENT_FOR_TOO_LONG,
	BILGE_CURRENT_ZERO,
	BILGE_OVERCURRENT,
	CURRENT_READ_ERROR,
	BILGE_AUTO_MODE_OFF,
	BILGE_TIMEOUT
} SBT_e_bilge_event;

typedef enum {
	BILGE_PUMP_ON,
	BILGE_PUMP_OFF,
	BILGE_PUMP_AUTO
} SBT_e_bilge_mode;

typedef struct {
	uint8_t bilge_current_zero;
	uint8_t bilge_overcurrent;
	uint8_t current_read_error;
	uint8_t water_detected_current;
	uint8_t water_detected_sensor;
} SBT_s_bilge_fatal_error;

typedef enum{
	BILGE_PUMP_AUTO_OFF,
	BILGE_PUMP_AUTO_ON,
} SBT_e_bilge_auto_action;


// -------------------------------------------- PUMP RELATED --------------------------------------------

/**
 * @brief Turns on/off pump in auto mode if water detected by current
 * 
 * @param bilge_fatal_error - struct with water_detected_by_current var
 * 
 * @retval ID of SBT_e_pump_auto_action
 * 
 */
int SBT_Bilge_Auto_Controll(SBT_s_bilge_fatal_error* bilge_fatal_error);


/**
 * @brief Calculates value of current in [mA] and pass it to other pump rel functions
 * 
 * @param bilge_parameters - obtained by ADC via DMA to unit16_t bilge_parameters[100] 
 * 
 * @param bilge_fatal_error - struct with errors indicators
 * 
 * @retval None
 */
void SBT_Bilge_State_Check(uint16_t bilge_parameters, SBT_s_bilge_fatal_error* bilge_fatal_error);


/**
 * @brief Check for water presence by pump's current value analysis.
 * 
 * @param pump_current - obtained by ADC via DMA to unit16_t bilge_parameters[100] 
 *
 * @param bilge_fatal_error - struct with water_detected_by_current var
 *
 * @return None
 */
void SBT_Bilge_Detect_Water_By_Current(uint16_t pump_current, SBT_s_bilge_fatal_error* bilge_fatal_error);


/**
 * @brief Analysis bilge pump errors
 * 
 * @param pump_current - obtained by ADC via DMA to unit16_t bilge_parameters[100]
 *
 * @param bilge_fatal_error - struct with errors indicators
 *
 * @retval None
 *
 */
void SBT_Bilge_Pump_Error_Analysis(uint16_t pump_current, SBT_s_bilge_fatal_error* bilge_fatal_error);


// -------------------------------------- WARER SENSOR RELATED ----------------------------------------
/**
 * @brief Indicates water presence by water sensor analysis
 * 
 * @param bilge_fatal_error - struct with water_detected_by_sensor var
 * 
 * @retval None
 */
void SBT_Bilge_Water_Sensor(SBT_s_bilge_fatal_error* bilge_fatal_error);


// ----------------------------------------- SYSTEM RELATED -------------------------------------------

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
void StartTaskBilgePump(void const * argument);


// Quick test delays
///////////////////////////////////////////////////////////////////////////////////////
// Timer .h  file
void delayUs(int16_t us);

void delayMs(int32_t ms);

float map(float xin, float xa, float xb, float ya, float yb);

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

// #define UNUSED(x) (void)(x)

//timer

// set lastExec to current time
void timerArm(uint32_t* lastExecTime);

//add delay to last execution time. Use olny after timer expired.
void timerArmAgain(uint32_t* lastExecTime, uint32_t delay_ms);

// return remaining time to complete delay ( before, positive
int64_t timerGet(uint32_t lastExecTime, uint32_t delay_ms );

//return remaining time or 0
uint32_t timerGetRemaining(uint32_t lastExecTime, uint32_t delay_ms );
///////////////////////////////////////////////////////////////////////////////////////

#endif /* INC_TASKPUMPCONTROLL_H_ */
