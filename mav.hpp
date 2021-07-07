#pragma once

#include <QObject>
#include <QTimer>
#include <string>
#include "mavlink/standard/mavlink.h"
#include <netinet/in.h>

class Mavlink : public QObject
{
    Q_OBJECT
public:
    explicit Mavlink(std::string ipaddr, QObject *parent = nullptr, uint8_t sysid = 1, uint8_t compid = 1);
    ~Mavlink();

    void send_msg(mavlink_message_t * msg);
    void sensor_alive(MAV_SYS_STATUS_SENSOR sensor_mask);
    /**
     * @brief Update Battery Info based on BMS data
     *
     * @param voltage_battery   [mV] Battery voltage, UINT16_MAX: Voltage not sent by autopilot
     * @param current_battery   [cA] Battery current, -1: Current not sent by autopilot
     * @param battery_remaining [%] Battery energy remaining, -1: Battery remaining energy not sent by autopilot
     */
    void update_battery_info(uint16_t voltage_battery,
                             int16_t  current_battery,
                             int8_t   battery_remaining);

    void send_buf(mavlink_channel_t chan, const uint8_t *buf, uint16_t len);
    uint64_t microsSinceEpoch();
private:

    void receive();
    void send_hearbeat();
    int sock;
    struct sockaddr_in gcAddr;
    struct sockaddr_in locAddr;
    QTimer m_timer_rx;
    QTimer m_timer_heartbeat;
    char target_ip[100];
    uint32_t m_sensor_status;
    uint16_t m_voltage_battery;
    int16_t  m_current_battery;
    int8_t   m_battery_remaining;

    // Intervals in ms
    static const int RX_INTERVAL = 100;
    static const int HEARTBEAT_INTERVAL = 1000;
    static const uint32_t m_present_sensors = 0
        | MAV_SYS_STATUS_SENSOR_3D_ACCEL
        | MAV_SYS_STATUS_AHRS
        | MAV_SYS_STATUS_SENSOR_BATTERY
        | MAV_SYS_STATUS_SENSOR_GPS
        | MAV_SYS_STATUS_SENSOR_ATTITUDE_STABILIZATION
        | MAV_SYS_STATUS_SENSOR_ANGULAR_RATE_CONTROL
        ;
};
