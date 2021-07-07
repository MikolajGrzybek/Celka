#include "serialport.hpp"

// Linux headers
#include <fcntl.h>      // file controls like O_RDWR
#include <errno.h>      // Error integer and strerror() function
//#include <termios.h>  // POSIX terminal control definitions
#include <unistd.h>     // write(), read(), close()

#include <QDebug>


SerialPort::SerialPort(const QString & name, QObject *parent):
    m_port_name(name)
{

}

bool SerialPort::open(QIODevice::OpenMode mode)
{
    // Open the serial port in read only mode
    setOpenMode(mode);
    const char * port_name = m_port_name.toStdString().c_str();
    m_fd = ::open(port_name, O_RDWR | O_NOCTTY );
    // m_fd = ::open(port_name, _O_RDONLY | O_NOCTTY );
    if (m_fd < 0)
    {
        qDebug() << "Error opening \""<< port_name <<"\". Do you have enought privileges?";
        return -1;
    }
    else
    {
        qDebug() << "\"" << port_name << "\" opened";
    }
    // Read in existing settings
    if (tcgetattr(m_fd, &m_tty) != 0)
    {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return false;
    }

    // set suitable flags
    auto BAUDRATE = B115200;
    /*
      BAUDRATE: Set bps rate
      CS8     : 8n1 (8bit,no parity,1 stopbit)
      CLOCAL  : local connection, no modem contol
      CREAD   : enable receiving characters
    */
    m_tty.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    m_tty.c_iflag = IGNPAR;
    m_tty.c_oflag = 0;

    // set input mode raw (non-canonical, no echo)
    m_tty.c_lflag = 0;

    int min_frame_width = 1;
    m_tty.c_cc[VTIME]    = 0;
    m_tty.c_cc[VMIN]     = min_frame_width;   /* blocking read */

    //m_tty.c_cc[VTIME] = 1;              // Always wait up to 0.1s for data, returning as soon as data is received.
    //m_tty.c_cc[VMIN]  = 0;

    tcflush(m_fd, TCIFLUSH);
    if (tcsetattr(m_fd, TCSANOW, &m_tty) != 0)
    {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        return false;
    }

    //flush?
    //m_is_receiving = true;
    //m_receiver = new std::thread(&SerialPort::loop_p_read, this);

    return true;
}

void SerialPort::close()
{
    ::close(m_fd);
    setOpenMode(QIODevice::NotOpen);
}

bool SerialPort::setPortName(const QString &port_name)
{
    return true;
}

bool SerialPort::setBaudRate(SerialPort::BaudRate)
{
    return true;
}

bool SerialPort::setDataBits(SerialPort::DataBits)
{
    return true;
}

bool SerialPort::setFlowControl(SerialPort::FlowControl)
{
    return true;
}

bool SerialPort::setParity(SerialPort::Parity)
{
    return true;
}

bool SerialPort::setStopBits(SerialPort::StopBits)
{
    return true;
}

void SerialPort::start_receiving()
{
    //flush port?
    m_is_receiving = true;
    m_receiver = new std::thread(&SerialPort::loop_p_read, this);
}

void SerialPort::stop_receiving()
{
    if (!m_is_receiving) return;

    m_is_receiving = false;
    m_receiver->join();
    delete m_receiver;
}

qint64 SerialPort::readData(char *data, qint64 maxSize)
{
    if (maxSize < 1) maxSize = 1;
    maxSize = maxSize > BUFF_SIZE ?
                BUFF_SIZE :
                maxSize;

    int dr = 0;
    while (dr < maxSize - 1)
    {
        data[dr] = m_buffor[dr];
        dr++;
    }
    data[dr] = '\0';

    qDebug() << __PRETTY_FUNCTION__ << data;
    return dr;
}

qint64 SerialPort::readLine(char *data, qint64 maxSize)
{
    if (maxSize < 1) maxSize = 1;
    maxSize = maxSize > BUFF_SIZE ?
                BUFF_SIZE :
                maxSize;
    int dr = 0;
    while (dr < maxSize - 1)
    {
        data[dr] = m_buffor[dr];
        if (m_buffor[dr] == '\n' || m_buffor[dr] == '\0') break;
        dr++;
    }
    data[++dr] = '\0';
    //qDebug() << __PRETTY_FUNCTION__ << data;
    return dr+1;
}

qint64 SerialPort::writeData(const char *data, qint64 maxSize)
{
    // todo
    return maxSize;
}

int SerialPort::p_read()
{
    int dr = ::read(m_fd, m_buffor, BUFF_SIZE - 1);
    if (dr < 0) perror("Serial read error");
    else
        emit readyRead();
    return dr;
}

void SerialPort::loop_p_read()
{
    while (m_is_receiving) p_read();
}
