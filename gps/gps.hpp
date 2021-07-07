#ifndef GPS_HPP
#define GPS_HPP

#include <QObject>

#include "serialport/serialport.hpp"
#include "msgs/NMEA.hpp"

Q_DECLARE_METATYPE(sbt_RMC_msg)

struct Pos{
    double lon;
    double lat;
};

class Gps : public QObject
{
    Q_OBJECT
public:
    explicit Gps(QString port_name, QObject *parent = nullptr);
    ~Gps();

    bool    is_message_valid() const;  // status
    double  get_speed()        const;  // km/h
    double  get_cog()          const;  // deg
    Pos     get_position()     const;  // deg, deg

signals:
    void fixChanged(bool is_fixed);
    void position_updated(sbt_RMC_msg msg);

private slots:
    void readLine();

private:
    sbt_RMC_msg m_msg;
    SerialPort* m_serial;

    void initialize();
};

#endif // GPS_HPP
