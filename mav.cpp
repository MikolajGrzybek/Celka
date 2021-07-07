#include "mav.hpp"

#include <QTimer>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include "mavlink/standard/mavlink.h"
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdbool.h> /* required for the definition of bool in C99 */

#define BUFFER_LENGTH 2041 // minimum buffer size that can be used with qnx (I don't know why)

static Mavlink * m_mav;
mavlink_system_t mavlink_system;
void send_buf_gmav(mavlink_channel_t chan, const uint8_t *buf, uint16_t len) {
    m_mav->send_buf(chan, buf, len);
}
uint64_t Mavlink::microsSinceEpoch()
{

    struct timeval tv;

    uint64_t micros = 0;

    gettimeofday(&tv, NULL);
    micros = ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

    return micros;
}
Mavlink::Mavlink(std::string ipaddr, QObject *parent, uint8_t sysid, uint8_t compid)
    : m_timer_rx(this), m_timer_heartbeat(this)
{
    m_mav = this;
    mavlink_system.sysid = sysid;
    mavlink_system.compid = compid;
    float position[6] = {};
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //struct sockaddr_in fromAddr;
    int bytes_sent;
    mavlink_message_t msg;
    //int success = 0;

    // Change the target ip if parameter was given
    strcpy(target_ip, ipaddr.c_str());

    memset(&locAddr, 0, sizeof(locAddr));
    locAddr.sin_family = AF_INET;
    locAddr.sin_addr.s_addr = INADDR_ANY;
    locAddr.sin_port = htons(14541);

    /* Bind the socket to port 14551 - necessary to receive packets from qgroundcontrol */
    if (-1 == bind(sock, (struct sockaddr *)&locAddr, sizeof(struct sockaddr)))
    {
        perror("error bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Attempt to make it non blocking */
    if (fcntl(sock, F_SETFL, O_NONBLOCK | O_ASYNC) < 0)
    {
        fprintf(stderr, "error setting nonblocking: %s\n", strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }

    memset(&gcAddr, 0, sizeof(gcAddr));
    gcAddr.sin_family = AF_INET;
    gcAddr.sin_addr.s_addr = inet_addr(target_ip);
    gcAddr.sin_port = htons(14551);

    connect(&m_timer_rx, &QTimer::timeout, this, &Mavlink::receive);
    m_timer_rx.start(RX_INTERVAL);
    connect(&m_timer_heartbeat, &QTimer::timeout, this, &Mavlink::send_hearbeat);
    m_timer_heartbeat.start(HEARTBEAT_INTERVAL);
}

void Mavlink::send_msg(mavlink_message_t * msg)
{
    uint8_t buf[BUFFER_LENGTH];
    auto len = mavlink_msg_to_send_buffer(buf, msg);
    sendto(sock, buf, len, 0, (struct sockaddr *)&gcAddr, sizeof(struct sockaddr_in));
}


void Mavlink::send_buf(mavlink_channel_t chan, const uint8_t *buf, uint16_t len)
{
    sendto(sock, buf, len, 0, (struct sockaddr *)&gcAddr, sizeof(struct sockaddr_in));
}

Mavlink::~Mavlink()
{
}

void Mavlink::receive() {
    unsigned int temp = 0;
    ssize_t recsize;
    socklen_t fromlen = sizeof(gcAddr);
    uint8_t buf[BUFFER_LENGTH];
    memset(buf, 0, BUFFER_LENGTH);
    recsize = recvfrom(sock, (void *)buf, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);
    if (recsize > 0)
    {
        // Something received - print out all bytes and parse packet
        mavlink_message_t msg;
        mavlink_status_t status;

        printf("Bytes Received: %d\nDatagram: ", (int)recsize);
        for (size_t i = 0; i < recsize; ++i)
        {
            temp = buf[i];
            printf("%02x ", (unsigned char)temp);
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status))
            {
                // Packet received
                printf("\nReceived packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
            }
        }
        printf("\n");
    }

}

void Mavlink::send_hearbeat() {
    printf("Send heartbeat\n");
    mavlink_msg_heartbeat_send(MAVLINK_COMM_0,
                               MAV_TYPE_SURFACE_BOAT,
                               MAV_AUTOPILOT_INVALID,
                               MAV_MODE_GUIDED_ARMED,
                               0,
                               MAV_STATE_ACTIVE);



	mavlink_msg_sys_status_send(MAVLINK_COMM_0,
                                m_present_sensors,
                                m_present_sensors,
                                m_sensor_status,
                                100, // system load
                                m_voltage_battery, // battery voltage mV
                                m_current_battery, // battery current
                                m_battery_remaining, // batery remaining
                                0, // drop rate
                                0, 0, 0, 0, 0);
    m_sensor_status = 0;
    update_battery_info(UINT16_MAX, -1, -1);
}

void Mavlink::sensor_alive(MAV_SYS_STATUS_SENSOR sensor_mask) {
    m_sensor_status |= sensor_mask;
}
void Mavlink::update_battery_info(uint16_t voltage_battery,
                                  int16_t  current_battery,
                                  int8_t   battery_remaining) {
    m_voltage_battery = voltage_battery;
    m_current_battery = current_battery;
    m_battery_remaining = battery_remaining;
}
