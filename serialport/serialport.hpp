#ifndef SERIALPORT_HPP
#define SERIALPORT_HPP

#include <thread>

#include <QIODevice>
#include <QString>

#include <termios.h>

class SerialPort : public QIODevice
{
    Q_OBJECT
public:
    SerialPort(const QString & name, QObject * parent = nullptr);
    SerialPort(QObject * parent = nullptr); //todo

    enum Direction { Input, Output, AllDirections };

    enum BaudRate {
        Baud9600   = 9600,
        /*...*/
        Baud115200 = 115200
    };
    enum DataBits { Data5, Data6, Data7, Data8, UnknownDataBits };
    enum FlowControl { NoFlowControl, HardwareControl, SoftwareControl, UnknownFlowControl };
    enum Parity { NoParity, EvenParity, OddParity, SpaceParity, MarkParity, UnknownParity };
    enum StopBits { OneStop, OneAndHalfStop, TwoStop, UnknownStopBits };

    bool open(QIODevice::OpenMode mode);
    void close();

    bool setPortName(const QString & port_name);
    bool setBaudRate(SerialPort::BaudRate);
    bool setDataBits(SerialPort::DataBits);
    bool setFlowControl(SerialPort::FlowControl);
    bool setParity(SerialPort::Parity);
    bool setStopBits(SerialPort::StopBits);

    void start_receiving();
    void stop_receiving();



    qint64 readData(char *data, qint64 maxSize) override;
    qint64 readLine(char *data, qint64 maxSize); // todelete
    qint64 writeData(const char *data, qint64 maxSize) override;
    
    int  get_fd() const {return m_fd;}
private:
    int    m_fd;
    termios m_tty;

    QString     m_port_name;
    long int    m_baud_rate;
    DataBits    m_data_bits;
    FlowControl m_flow_control;
    Parity      m_parity;
    StopBits    m_stop_bits;

    bool  m_is_receiving = false;
    int   p_read();
    void  loop_p_read();

    static constexpr int BUFF_SIZE = 2048; // tmp
    char m_buffor[BUFF_SIZE] = {};

    std::thread *m_receiver;
};

#endif // SERIALPORT_HPP
