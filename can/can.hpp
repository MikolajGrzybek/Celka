#ifndef CAN_HPP
#define CAN_HPP

#include <thread>
#include <fstream>

#include <QObject>

#include <QString>
#include <QDebug>

#include <linux/can.h>
#include <linux/can/raw.h>

Q_DECLARE_METATYPE(can_frame)

class CAN : public QObject
{
    Q_OBJECT
public:
    explicit CAN(QString can_interface, QObject *parent = nullptr);

    bool send_frame(int id, int dlc, int * data);
    can_frame get_frame();
    void start_receiving();
    void stop_receiving();

signals:
    void frame_received(can_frame cf);
//public slots:

protected:
    void setup(const char* ifname);

    int write(int id, int dlc, int* data);
    int read();
    int loop_read();
private:
    can_frame *m_frame;
    int        m_socket;

    bool        m_is_on = false;
    std::thread *m_receiver;

    std::fstream m_file;
};

#endif // CAN_HPP
