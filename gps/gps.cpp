#include "gps.hpp"

#include <QTimer>
#include <QDebug>
#include <unistd.h>
// SBT
#include "parser.hpp"

#include "gps_init.h"

DEFINE_MSG(INIT_CMD_00, 0)
DEFINE_MSG(INIT_CMD_00, 1)
DEFINE_MSG(INIT_CMD_00, 2)
DEFINE_MSG(INIT_CMD_00, 3)
DEFINE_MSG(INIT_CMD_00, 4)
DECLARE_MSG_ARRAY(INIT_CMD_00, 5)

DEFINE_MSG(INIT_CMD_01, 0)
DEFINE_MSG(INIT_CMD_01, 1)
DEFINE_MSG(INIT_CMD_01, 2)
DEFINE_MSG(INIT_CMD_01, 3)
DEFINE_MSG(INIT_CMD_01, 4)
DECLARE_MSG_ARRAY(INIT_CMD_01, 5)

DEFINE_MSG(INIT_CMD_02, 0)
DEFINE_MSG(INIT_CMD_02, 1)
DEFINE_MSG(INIT_CMD_02, 2)
DEFINE_MSG(INIT_CMD_02, 3)
DEFINE_MSG(INIT_CMD_02, 4)
DECLARE_MSG_ARRAY(INIT_CMD_02, 5)

DEFINE_MSG(INIT_CMD_03, 0)
DEFINE_MSG(INIT_CMD_03, 1)
DEFINE_MSG(INIT_CMD_03, 2)
DEFINE_MSG(INIT_CMD_03, 3)
DEFINE_MSG(INIT_CMD_03, 4)
DECLARE_MSG_ARRAY(INIT_CMD_03, 5)

DEFINE_MSG(INIT_CMD_04, 0)
DEFINE_MSG(INIT_CMD_04, 1)
DEFINE_MSG(INIT_CMD_04, 2)
DEFINE_MSG(INIT_CMD_04, 3)
DEFINE_MSG(INIT_CMD_04, 4)
DECLARE_MSG_ARRAY(INIT_CMD_04, 5)

DEFINE_MSG(INIT_CMD_05, 0)
DEFINE_MSG(INIT_CMD_05, 1)
DECLARE_MSG_ARRAY(INIT_CMD_05, 2)

static gps_init_cmd init_cmd_array[INIT_CMD_NUM];
static void fill_init_cmd_array() {
    CMD_MSG_ARRAY(INIT_CMD_00)[0] = CMD_MSG(INIT_CMD_00, 0);
    CMD_MSG_ARRAY(INIT_CMD_00)[1] = CMD_MSG(INIT_CMD_00, 1);
    CMD_MSG_ARRAY(INIT_CMD_00)[2] = CMD_MSG(INIT_CMD_00, 2);
    CMD_MSG_ARRAY(INIT_CMD_00)[3] = CMD_MSG(INIT_CMD_00, 3);
    CMD_MSG_ARRAY(INIT_CMD_00)[4] = CMD_MSG(INIT_CMD_00, 4);
    init_cmd_array[0].m_num = 5;
    init_cmd_array[0].m_messages = CMD_MSG_ARRAY(INIT_CMD_00);

    CMD_MSG_ARRAY(INIT_CMD_01)[0] = CMD_MSG(INIT_CMD_01, 0);
    CMD_MSG_ARRAY(INIT_CMD_01)[1] = CMD_MSG(INIT_CMD_01, 1);
    CMD_MSG_ARRAY(INIT_CMD_01)[2] = CMD_MSG(INIT_CMD_01, 2);
    CMD_MSG_ARRAY(INIT_CMD_01)[3] = CMD_MSG(INIT_CMD_01, 3);
    CMD_MSG_ARRAY(INIT_CMD_01)[4] = CMD_MSG(INIT_CMD_01, 4);
    init_cmd_array[1].m_num = 5;
    init_cmd_array[1].m_messages = CMD_MSG_ARRAY(INIT_CMD_01);

    CMD_MSG_ARRAY(INIT_CMD_02)[0] = CMD_MSG(INIT_CMD_02, 0);
    CMD_MSG_ARRAY(INIT_CMD_02)[1] = CMD_MSG(INIT_CMD_02, 1);
    CMD_MSG_ARRAY(INIT_CMD_02)[2] = CMD_MSG(INIT_CMD_02, 2);
    CMD_MSG_ARRAY(INIT_CMD_02)[3] = CMD_MSG(INIT_CMD_02, 3);
    CMD_MSG_ARRAY(INIT_CMD_02)[4] = CMD_MSG(INIT_CMD_02, 4);
    init_cmd_array[2].m_num = 5;
    init_cmd_array[2].m_messages = CMD_MSG_ARRAY(INIT_CMD_02);

    CMD_MSG_ARRAY(INIT_CMD_03)[0] = CMD_MSG(INIT_CMD_03, 0);
    CMD_MSG_ARRAY(INIT_CMD_03)[1] = CMD_MSG(INIT_CMD_03, 1);
    CMD_MSG_ARRAY(INIT_CMD_03)[2] = CMD_MSG(INIT_CMD_03, 2);
    CMD_MSG_ARRAY(INIT_CMD_03)[3] = CMD_MSG(INIT_CMD_03, 3);
    CMD_MSG_ARRAY(INIT_CMD_03)[4] = CMD_MSG(INIT_CMD_03, 4);
    init_cmd_array[3].m_num = 5;
    init_cmd_array[3].m_messages = CMD_MSG_ARRAY(INIT_CMD_03);

    CMD_MSG_ARRAY(INIT_CMD_04)[0] = CMD_MSG(INIT_CMD_04, 0);
    CMD_MSG_ARRAY(INIT_CMD_04)[1] = CMD_MSG(INIT_CMD_04, 1);
    CMD_MSG_ARRAY(INIT_CMD_04)[2] = CMD_MSG(INIT_CMD_04, 2);
    CMD_MSG_ARRAY(INIT_CMD_04)[3] = CMD_MSG(INIT_CMD_04, 3);
    CMD_MSG_ARRAY(INIT_CMD_04)[4] = CMD_MSG(INIT_CMD_04, 4);
    init_cmd_array[4].m_num = 5;
    init_cmd_array[4].m_messages = CMD_MSG_ARRAY(INIT_CMD_04);

    CMD_MSG_ARRAY(INIT_CMD_05)[0] = CMD_MSG(INIT_CMD_05, 0);
    CMD_MSG_ARRAY(INIT_CMD_05)[1] = CMD_MSG(INIT_CMD_05, 1);
    init_cmd_array[5].m_num = 2;
    init_cmd_array[5].m_messages = CMD_MSG_ARRAY(INIT_CMD_05);
}


Gps::Gps(QString port_name, QObject *parent):
    QObject(parent),
    m_serial(new SerialPort(port_name))
{
    qRegisterMetaType<sbt_RMC_msg>();

    // "/dev/ttyACM0", 115200
    m_serial->setPortName("ttyACM0");
    m_serial->setBaudRate(SerialPort::Baud115200);
    m_serial->setDataBits(SerialPort::Data8);
    m_serial->setParity(SerialPort::NoParity);
    m_serial->setStopBits(SerialPort::OneStop);
    m_serial->setFlowControl(SerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadOnly)){
        qDebug() << "Serial open errror!";
    }
    else
    {
        //connect(m_serial, &SerialPort::readyRead, this, &Gps::read_message);
        const int HZ = 10;                  // [1/s]
        const int timeInterval = 1000 / HZ; // [ms]
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Gps::readLine);
        timer->start(timeInterval);
        qDebug() << "Gps Conected";
        initialize();
    }
}

Gps::~Gps()
{
    //if (m_serial->isOpen())
    //    m_serial->close();
    delete m_serial;
}

void Gps::initialize()
{
    // Init GPS
    fill_init_cmd_array();
    for (int i = 0; i < INIT_CMD_NUM; i++) {
        for (int j = 0; j < init_cmd_array[i].m_num; j++) {

            size_t dw = write(m_serial->get_fd(), (const char *)init_cmd_array[i].m_messages[j].m_buff, init_cmd_array[i].m_messages[j].m_len);
            if (dw == -1)
                printf("Error %i from write: %s\n", errno, strerror(errno));
            else if (init_cmd_array[i].m_messages[j].m_len != dw)
                printf("Sending %lu out of %lu", dw, init_cmd_array[i].m_messages[j].m_len);
            else
            {
                printf("Msg sent");
            }
        }
    }
    puts("");
}

void Gps::readLine()
{
    //M.M.
    static char line_buf[256];
    static char rx_buf[256];
    static size_t line_buf_pos = 0;
    int dr = read(m_serial->get_fd(), rx_buf, 256);
    for (int i = 0; i < dr; i++) {
        line_buf[line_buf_pos] = rx_buf[i];
        if (line_buf[line_buf_pos] == '\n') {

            sbt_RMC_msg tmp_msg;
            if (Parser::parse_RMC(line_buf, tmp_msg) == true)
            {
                const bool new_status = tmp_msg.status;
                if (new_status != m_msg.status)
                    emit fixChanged(new_status);
                m_msg = tmp_msg;
                emit position_updated(m_msg);
            }
            line_buf[line_buf_pos + 1] = 0;
            line_buf_pos = 0;
        } else {
            line_buf_pos++;
        }
    }
}

bool Gps::is_message_valid() const
{
    return m_msg.status;
}

double Gps::get_speed() const
{
    return m_msg.speed;
}

double Gps::get_cog() const
{
    return m_msg.cog;
}

Pos Gps::get_position() const
{
    Pos pos;
    pos.lat = m_msg.Position_t.lat;
    pos.lon = m_msg.Position_t.lon;

    return pos;
}
