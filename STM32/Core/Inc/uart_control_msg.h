/*
 * uart_control_msg.h
 *
 *  Created on: 9 gru 2020
 *      Author: Karol
 */

#ifndef INC_UART_CONTROL_MSG_H_
#define INC_UART_CONTROL_MSG_H_

typedef enum {
    MOTOR_ID,
    IMU_ID,
    XBEE_ID,
    ULTRASONIC_ID,
    BATTERY_ID,
    ERROR_ID,
	PUMP_ID,
	PUMP_MODE_ID
} SBT_ctrl_msg_id_t;

typedef enum{
    WRONG_COMMAND,
    IMU_ERROR,
    XBEE_ERROR,
    ULTRASONIC_ERROR,
    BATTERY_ERROR,
} SBT_ctrl_error_msg_t;

#pragma pack(push, 1)


typedef struct{
    uint16_t imu_roll;
    uint16_t imu_pitch;
    uint16_t imu_yaw;
    uint16_t imu_acc_x;
    uint16_t imu_acc_y;
    uint16_t imu_acc_z;
} SBT_imu_msg_t;

typedef struct{
    uint16_t ultrasonic_left;
    uint16_t ultrasonic_right;
} SBT_ultrasonic_msg_t;

typedef struct{
    uint8_t battery_level;
} SBT_battery_msg_t;

// TODO
typedef struct{
    uint16_t xbee_data;
} SBT_xbee_msg_t;

#pragma pack(pop)



#endif /* INC_UART_CONTROL_MSG_H_ */
