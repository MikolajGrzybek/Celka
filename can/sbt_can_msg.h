/*
 * https://gitlab.com/AghSolarBoat2017/Software/-/blob/master/2019_2020/Lodzie/New/Control_Board_1v1/Inc/sbt_can_msg.h
 *
 * can_and_inter_thread_structures.h
 *
 *  Created on: 3 mar 2021
 *      Author: Karol
 */

#ifndef INC_CAN_AND_INTER_THREAD_STRUCTURES_H_
#define INC_CAN_AND_INTER_THREAD_STRUCTURES_H_

#include <cinttypes>
//#include "main.h"


/*
 * CAN CONSTANTS
 *
 */

#define MAX_AVAILABLE_ID 10
#define MIN_AVAILABLE_ID 0

#define CAN_MSG_DEFAULT_LNG 8

#define CAN_ID_OFFSET 6

typedef enum{

	COOLING_SYSTEM_PARAMETERS_ID = CAN_ID_OFFSET, // flow meter and motor temperature
	PUMP_MODE_ID,     			  		  		  // cooling pump mode - AUTO, ON, OFF {COOLING, AUTO BILGE, BILGE1 BILGE2}
	COOLING_SYSTEM_ERRORS_ID,						  // problems with overheat, dry work, leak
	MOTOR_POWER_ID,								  // motor power from potentiometer
	HYDROFOIL_ANGLE_ID,							  // angle set by user
	BILGE_SYSTEM_ERRORS_ID,						  // bilge pump problems
	IMU_ANGLES_ID,								  // roll pitch yaw
	IMU_ACC_ID,									  // x, y, z acc
	BMS_DATA_ID,								  // cell voltage (min, max), discharge current, temp
	SPEED_ID,									  // to be done
	GPS_ID										  // to be done

} can_msg_id_t;


/*
 * ONLY CAN STRUCTURES
 */

/*
 * generic can queue
 * used only in task can
 *
 */
typedef struct {
	uint16_t msg_id;
	uint8_t message[8];
} can_generic_struct;

// --------------MOTOR AND ACTUATORS ---------------

typedef struct {
	int16_t potentiometer_value1;
	int16_t potentiometer_value2;
	int16_t unused_1;
	int16_t unused_2;
} can_motor_power;

typedef struct {
	int16_t left_foil_angle;
	int16_t right_foil_angle;
	int16_t unused_1;
	int16_t unused_2;
} can_hydrofoil_angle;

// --------- IMU ------------

typedef struct{
	int16_t imu_acc_x;
	int16_t imu_acc_y;
	int16_t imu_acc_z;
} can_imu_acc;

typedef struct{
	int16_t imu_roll;
	int16_t imu_pitch;
	int16_t imu_yaw;
} can_imu_angles;


// -------------------- BMS ---------------

typedef struct{
	uint16_t max_cell_volatage;
	uint16_t min_cell_voltage;
	uint16_t discharge_cuttent;
	uint16_t charge_current;
} can_bms_data;

// ------------------ COOLING SYSTEM ------------------
typedef struct {
    int16_t motor_one_temp;		//temperature from analog sensor
    int16_t motor_two_temp;		//temperature from analog sensor
    int16_t input_flow;
    int16_t output_flow;
} can_pump_state_input;

typedef enum {
	OVERHEAT,					// too high motor temperature
	DRY_RUNNING,				// no water in cooling system
	LEAK,						// detected leakage in cooling system
	NONE,						// no error
	TIMEOUT						// task failed to read cooling system parameters in specified time
} SBT_e_pump_alarm;

typedef enum {
	PUMP_ON,
	PUMP_OFF,
	PUMP_AUTO
} SBT_e_pump_mode;


// ------------------ BILGE PUMP SYSTEM --------------
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

#endif /* INC_CAN_AND_INTER_THREAD_STRUCTURES_H_ */
