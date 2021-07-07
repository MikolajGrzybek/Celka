#include "can.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>

#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>

#include "utils.h"

CAN::CAN(QString can_interface, QObject *parent):
    m_frame(new can_frame)
{
    qRegisterMetaType<can_frame>();
    setup(can_interface.toStdString().c_str());
}

bool CAN::send_frame(int id, int dlc, int* data)
{
    return write(id, dlc, data);
}

can_frame CAN::get_frame()
{
    return *m_frame;
}

void CAN::start_receiving()
{
    
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,sizeof(buffer),"/home/pi/logs/celka_log_%Y_%m_%d_%H_%M.txt",timeinfo);
    std::string file_name(buffer);

    m_file.open(file_name, std::ios::out | std::ios::app);

    //flush socket?
    m_is_on = true;
    m_receiver = new std::thread(&CAN::loop_read, this);
}

void CAN::stop_receiving()
{
    if (!m_is_on) return;

    m_is_on = false;
    m_receiver->join();
    delete m_receiver;
    m_file.close();
}

/////////////////////////////////////////////////////////////////////
/**
 * @brief           Open and bind socket to struct sockaddr_can
 *
 * @param ifname    interface name e.g: virtual - "vcan0", phisical - "can0"
 */
void CAN::setup(const char* ifname)
{
    m_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_socket < 0)   throw "Socket not opened";

    // Bind the socket to the interface under
    struct sockaddr_can addr;
    struct ifreq ifr;

    strcpy(ifr.ifr_name, ifname);
    ioctl(m_socket, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(m_socket, (struct sockaddr *)&addr, sizeof(addr));
}
/////////////////////////////////////////////////////////////////////

/**
 * @brief       Send single can frame
 *
 * @param id    CAN id
 * @param dlc   CAN dlc - data length count
 * @param data  CAN frame payload
 * @return int  status
 */
int CAN::write(int id, int dlc, int *data)
{
    dlc = dlc > 8? 8 : dlc;

    can_frame *frame = new can_frame;
    frame->can_id=id;
    frame->can_dlc=dlc;

    int i = 0;
    while (i < dlc)
    {
        frame->data[i] = data[i];
        i++;
    }

    int nbytes = ::write(m_socket, frame, sizeof(can_frame));
    if (nbytes < 0) {
            perror("CAN raw socket write - nbytes < 0");
            return 1;
    }
    
    char buff[100];
    sprint_canframe(buff, m_frame, 0);
    auto start = std::chrono::high_resolution_clock::now();
    m_file << std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()).count();
    m_file << " " << buff << std::endl;

    return 0;
}
/////////////////////////////////////////////////////////////////////

int CAN::read()
{
    // Reading CAN frames:
    // qDebug() << __PRETTY_FUNCTION__ << "Waiting";
    int nbytes = ::read(m_socket, m_frame, sizeof(can_frame));
    if (nbytes < 0) {
            perror("CAN raw socket read - nbytes < 0");
            qDebug() << "Error read";
            return 1;
    }
    /* paranoid check ... */
    if (nbytes < sizeof(can_frame)) {
            qDebug() << "ReadCAN: incomplete CAN frame\n";
            return 1;
    }
    //Extract m_frame.id, m_frame.can_dlc, m_frame.data[i]
    char buff[100];
    sprint_canframe(buff, m_frame, 0);
    qDebug() << buff;
    auto start = std::chrono::high_resolution_clock::now();
    m_file << std::chrono::duration_cast<std::chrono::nanoseconds>(start.time_since_epoch()).count();
    m_file << " " << buff << std::endl;

    emit frame_received(*m_frame);

    return 0;
}

int CAN::loop_read()
{
    while(m_is_on)  this->read();
    // ...
    return 0;
}