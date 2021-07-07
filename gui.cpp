#include "gui.hpp"
#include "./ui_gui.h"

#include <QFile>
#include <QDebug>

#include <QJsonArray>
#include <QJsonDocument>

//#include <QString>
#include "can/sbt_can_msg.h"
#include "can/utils.h"
#include "mav.hpp"
#include "mavlink/standard/mavlink.h"

static const double PI = 3.14159265359;
static const double DEG2RAD = (180.0 / PI);
enum Pages : char
{
    MAIN        = 0,
    TM          = 1,
    SETTINGS    = 2
};

Gui::Gui(QWidget *parent)
    : QWidget(parent),
     gps(new Gps(GPS_PORT, parent)),
     m_can(new CAN(CAN_ITF ,parent)),
     m_mav("10.0.0.1", parent),
     ui(new Ui::Gui)
{
    setWindowTitle("GUI");
    setWindowIcon(QIcon(":/logoSolar.png"));
    setBackgroundRole(QPalette::Dark);

    m_start_time = std::chrono::high_resolution_clock::now();

    ui->setupUi(this);
    ui->pageManager->setCurrentIndex(MAIN);

    load_settings();

    // init pomps
    m_pumps = {.main_p  = PUMP_OFF,
               .side_1  = PUMP_OFF,
               .side_2  = PUMP_OFF,
               .cooling = PUMP_OFF
              };

    // init parameters in settings page
    int i = 0;
    const QStringList keys = m_parameters.keys();
    const int wd = keys.size();
    for ( const auto & name : keys)
    {
        auto value = m_parameters.value(name).toDouble();
        auto pname = QString("%1").arg(value, 4, 'g', 6);
//        auto * p = new QPushButton(pname, this);
        auto * p = new QLabel(pname, this);
        qDebug() << "Param: " << name << pname;
        p->setMaximumWidth(50);
        p->setMinimumHeight(15);
        m_parameters_list.push_back(p);
        if (i < (wd / 3))
            ui->fl_left->addRow(name, p);
        else if (i < (2 * wd / 3))
            ui->fl_middle->addRow(name, p);
        else
            ui->fl_right->addRow(name, p);
        i++;
    }

    m_can->start_receiving();
    connect(m_can, &CAN::frame_received, this, &Gui::on_can_frame_received);

    connect(gps, &Gps::position_updated, this, &Gui::on_gps_data_received);
    connect(gps, &Gps::fixChanged, this, &Gui::on_gps_fix_changed);


}

Gui::~Gui()
{
    delete ui;
}

void Gui::on_bHome_clicked()
{
    ui->pageManager->setCurrentIndex(Pages::MAIN);

}

void Gui::on_bPage2_clicked()
{
    ui->pageManager->setCurrentIndex(Pages::TM);
}

void Gui::on_bSettings_clicked()
{
    ui->pageManager->setCurrentIndex(Pages::SETTINGS);
}

extern Mavlink * g_mav;
void Gui::on_gps_data_received(sbt_RMC_msg msg)
{
    double speed  = msg.speed;
    int speed_int = (int)speed;
    int* data = &speed_int;
    constexpr float SPEED_OFFSET = 0.1f;  // floating reduction
    if (speed < SPEED_OFFSET)
        speed = 0.00f;

    ui->speedGps->display(speed);
    m_can->send_frame(can_msg_id_t::GPS_ID, 4, data);
    mavlink_msg_gps_raw_int_send(MAVLINK_COMM_0, m_mav.microsSinceEpoch(), 3,
								 msg.Position_t.lat * 1e7,
								 msg.Position_t.lon * 1e7,
								 30000, //altitude
								 UINT16_MAX, // hdop
								 UINT16_MAX,
								 (msg.speed/3.6)*100, // speed
								 msg.cog*10.0, //cog
								 UINT8_MAX);
    m_mav.sensor_alive(MAV_SYS_STATUS_SENSOR_GPS);
}

void Gui::on_gps_fix_changed(bool is_fixed)
{
    QString txt = "GPS " + (is_fixed? "" : QString("NO ")) + "FIX";
    this->ui->lGpsFix->setText(txt);
    if (is_fixed)
        this->ui->signal_icon->setStyleSheet("border-image:url(:gps/fix.png) 2 5 2 5;");
    else
        this->ui->signal_icon->setStyleSheet("border-image:url(:gps/no_fix.png) 2 5 2 5;");
}

void Gui::on_can_frame_received(can_frame cf)
{
    char frame_buff[100];
    sprint_canframe(frame_buff, &cf, ' ');
    QString frame_str(frame_buff);
    // tmp log
    ui->tb_logs->append(frame_buff);

    QStringList id_names = {
        "COOLING SYSTEM PARAMETERS",
        "PUMP MODE",
        "COOLING SYSTEM ERRORS",
        "MOTOR POWER",
        "HYDROFOIL ANGLE",
        "BILGE SYSTEM ERRORS",
        "IMU ANGLES",
        "IMU ACC",
        "BMS DATA",
        "SPEED",
        "GPS",
    };
    QStringList alarm_e {"OVERHEAT",  // too high motor temperature
                "DRY RUNNING",			// no water in cooling system
                "LEAK",					// detected leakage in cooling system
                "NONE",					// no error
                "TIMEOUT"};             // task failed to read cooling system parameters in specified time

    // log whole frame
    const int can_ID = cf.can_id;
    //const QString date = QDateTime::currentDateTime().toString("hh:mm:ss");
    const QString date = "[--]";
    // other id
    if (can_ID > 16 || can_ID < 6)
    {
        ui->tb_logs->setTextColor(Qt::yellow);
        ui->tb_logs->append(date + QString(" [%1]").arg(can_ID, 2) + " - Unknown message id");
        ui->tb_logs->setTextColor(Qt::black);

        return;
    }
    const QString id_string = id_names[can_ID - CAN_ID_OFFSET];
    const QString log_data = date + QString(" [%1]").arg(can_ID, 2) + " - " + id_string;
    //if error - todo | errors at the end of enum???

    // print on screen
    //ui->tb_errors->setText(log_data) -> nadpisanie wszystkiego
    // if recived error: /* to be improved */
    if (can_ID == 8 || can_ID == 11)
    {
        ui->tb_logs->setTextColor(Qt::red);
    }
    else
        ui->tb_logs->setTextColor(Qt::black);
    ui->tb_logs->append(log_data);

    // to do : odczytanie parametrów
    switch (can_ID) {
    case can_msg_id_t::COOLING_SYSTEM_PARAMETERS_ID:
    {
        can_pump_state_input psi;
        memcpy(&psi, cf.data, sizeof(can_pump_state_input));
        double mot1_temp = psi.motor_one_temp;
        double mot2_temp = psi.motor_two_temp;
        ui->pb_temp_1->setFormat(QString::number(mot1_temp) + " C");
        ui->pb_temp_2->setFormat(QString::number(mot2_temp) + " C");

        qDebug() << "Mot temp: " << mot1_temp;
//        psi.input_flow;  // unused
//        psi.output_flow; // unused
        break;
    }
    case can_msg_id_t::PUMP_MODE_ID:
        // only to send - souldnt be received
        break;
    case can_msg_id_t::COOLING_SYSTEM_ERRORS_ID:
        // SBT_e_pump_alarm?
        ui->tb_errors->append(date + " " + id_string + " -> " + alarm_e[cf.data[0]] + "!");
        break;
    case can_msg_id_t::MOTOR_POWER_ID:
    {
        can_motor_power mp;
        memcpy(&mp, cf.data, sizeof(can_motor_power));
        double throttle = mp.potentiometer_value1/20.f + 50;
        ui->pb_poten1->setValue(throttle);
        ui->pb_poten2->setValue(mp.potentiometer_value2);
//        // yyy
//        ui->pb_temp_1->setValue(mp.unused_1);
//        ui->pb_temp_2->setValue(mp.unused_2);
        break;
    }
    case can_msg_id_t::HYDROFOIL_ANGLE_ID:
        can_hydrofoil_angle ha;
        memcpy(&ha, cf.data, sizeof(can_hydrofoil_angle));
//        int16_t left_foil_angle;
//        int16_t right_foil_angle;
//        int16_t unused_1;
//        int16_t unused_2;
        break;

    case can_msg_id_t::BILGE_SYSTEM_ERRORS_ID:
        // ...
        // Coś z 'SBT_e_bilge_event' ?
        ui->tb_errors->append(date + " " + id_string + " -> " /* + alarm_e[cf.data[0]] */ + "!");
        break;

    case can_msg_id_t::IMU_ANGLES_ID: {
        can_imu_angles iang;
        memcpy(&iang, cf.data, sizeof(iang));
        m_mav.sensor_alive(MAV_SYS_STATUS_AHRS);
        mavlink_msg_attitude_send(MAVLINK_COMM_0,
                                  get_program_ticks(),
                                  (iang.imu_roll /100.f) * DEG2RAD,
                                  (iang.imu_pitch /100.f) * DEG2RAD,
                                  (iang.imu_yaw /100.f) * DEG2RAD,
                                  0, 0, 0); // Angular speed
        break;
    }

    case can_msg_id_t::IMU_ACC_ID: {
        can_imu_acc iacc;
        memcpy(&iacc, cf.data, sizeof(can_imu_acc));
        m_mav.sensor_alive(MAV_SYS_STATUS_SENSOR_3D_ACCEL);
        mavlink_msg_scaled_imu_send(MAVLINK_COMM_0,
                                    get_program_ticks(),
                                    iacc.imu_acc_x /100.f,
                                    iacc.imu_acc_x /100.f,
                                    iacc.imu_acc_x /100.f,
                                    0, 0, 0, // angular speed
                                    0, 0, 0); // Magnetometer
        break;
    }

    case can_msg_id_t::BMS_DATA_ID:
    {
        can_bms_data bmsd;
        memcpy(&bmsd, cf.data, sizeof(can_bms_data));
        double min_vol = bmsd.max_cell_volatage / 10000.f;
        double max_vol = bmsd.min_cell_voltage / 10000.f;
        double dis_cur = bmsd.discharge_cuttent / 100.f;
        double charge_cur = bmsd.charge_current/ 10000.f;
        ui->lcd_max_vol->display(max_vol);
        ui->lcd_min_vol->display(min_vol);
        ui->lcd_max_curr->display(dis_cur);
        ui->lcd_min_curr->display(charge_cur);
        m_mav.sensor_alive(MAV_SYS_STATUS_SENSOR_BATTERY);
        m_mav.update_battery_info((bmsd.min_cell_voltage * 12) / 10, bmsd.discharge_cuttent, -1);
        uint16_t cell_volts[10] = {
            (uint16_t) (bmsd.min_cell_voltage / 10),
            UINT16_MAX,
            UINT16_MAX,
            UINT16_MAX,
            UINT16_MAX,
            UINT16_MAX,
            UINT16_MAX,
            UINT16_MAX,
            UINT16_MAX,
            UINT16_MAX
        };
        mavlink_msg_battery_status_send(MAVLINK_COMM_0, 0,
                                        MAV_BATTERY_FUNCTION_ALL,
                                        MAV_BATTERY_TYPE_LION,
                                        INT16_MAX,       // [cDeg]temperature
                                        cell_volts, // [mV]  Cell voltages
                                        bmsd.discharge_cuttent,   // [cA]  Discharge Current
                                        -1,  // [mAh] Consumed charge
                                        bmsd.charge_current,       // [cA]  Charge current (custom by us)
                                        -1);        // [%]   Battery remaining
        break;
    }

    case can_msg_id_t::SPEED_ID:
        // ...
        break;
    case can_msg_id_t::GPS_ID:
        // ...
        break;
    default:
        ui->tb_logs->append(QString::number(can_ID) + " - not defined");
        break;
    }
}

// załadowanie parametrow
bool Gui::load_settings()
{
       // pomps loading - old
//    if (!m_other_pomps_list.isEmpty()) return false;

//    QFile loadF("../Gui/settings/pomps.list");

//    if (!loadF.open(QIODevice::ReadOnly))
//    {
//        qWarning("Couldn't open save file.");
//        return false;
//    }

//    char buffer[100];
//    qint64 dr = 0;

//    dr = loadF.readLine(buffer, 100);
//    while (dr > 0)
//    {
//        buffer[dr-1] = '\0'; // pozbycie sie '\n'
//        // załadowanie przyciskow od pomp
//        QPushButton * p = new QPushButton(QString(buffer), this);
//        p->setCheckable(true);
//        p->setMinimumSize(200, 50);
//        m_other_pomps_list.push_back(p);
//        ui->gb_other_pomps->layout()->addWidget(p);
//        qDebug() << buffer; // print pomp name
//        memset(buffer, '\0', 100);
//        dr = loadF.readLine(buffer, 100);
//    }

    if (!load_parameters())
        return false;

    return true;
}

bool Gui::load_parameters()
{
    QFile loadFile("../Gui/settings/parameters.json");

    if (!loadFile.open(QIODevice::ReadOnly))
    {
        qWarning("Couldn't open save file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));


    auto parameters = loadDoc.object()["parameters"].toObject();

    const QStringList klist = parameters.keys();
    const int size = parameters.size();

    for (int i = 0; i < size; i++)
    {
        auto & name = klist[i];
        auto value = parameters[name].toDouble();

        m_parameters[name] = value;
    }

    update_ui();

    return true;
}

bool Gui::save_parameters() const
{
    QFile saveFile("../Gui/settings/parameters.json");

    if (!saveFile.open(QIODevice::WriteOnly))
    {
        qWarning("Couldn't open file.");
        return false;
    }

    QJsonObject json_file;
    QJsonObject parameters;

    const QStringList klist = m_parameters.keys();
    const int p_size = m_parameters_list.size();
    for (int i = 0; i < p_size; ++i)
    {
        const auto & name = klist[i];
        parameters[name] = m_parameters[name].toDouble();
    }

    json_file["parameters"] = parameters;

    QJsonDocument saveDoc(json_file);
    saveFile.write(saveDoc.toJson());

    return true;
}

void Gui::update_ui()
{
    const QStringList klist = m_parameters.keys();

    int i = 0;
    for ( auto * el : m_parameters_list)
    {
        auto name = klist[i++];
        auto value = m_parameters.value(name).toDouble();
        auto strvalue = QString("%1").arg(value, 4, 'g', 6);
        el->setText(strvalue);
    }

}


static const QStringList modes{"ON", "OFF", "AUTO"};
void Gui::on_sbPomps_clicked()
{
    const QString core{ "MAIN POMP - " };

    const int enum_size = modes.size();
    const int new_pump_mode_id = (static_cast<int>(m_pumps.main_p) + 1) % enum_size;

    m_pumps.main_p = new_pump_mode_id;
int* data = m_pumps.data();
    m_can->send_frame(can_msg_id_t::PUMP_MODE_ID, 4, data);
delete[] data;

    ui->sbPomps->setText(core + modes[new_pump_mode_id]);
    ui->tb_logs->append("on_sbPomps_clicked() -> " + modes[new_pump_mode_id]);
}

void Gui::on_sbCooling_clicked(bool checked)
{
    const QString core{ "COOLING - " };

    const int enum_size = modes.size();
    const int new_pump_mode_id = (static_cast<int>(m_pumps.cooling) + 1) % enum_size;

    m_pumps.cooling = new_pump_mode_id;

int* data = m_pumps.data();
    m_can->send_frame(can_msg_id_t::PUMP_MODE_ID, 4, data);
delete[] data;
    ui->sbCooling->setText(core + modes[new_pump_mode_id]);
}

void Gui::on_sbSide_1_clicked(bool checked)
{
    ui->tb_logs->append(QString("on_sbSide_1_clicked() -> ") + (checked? "ON":"OFF"));
    if (checked)
        m_pumps.side_1 = SBT_e_pump_mode::PUMP_ON;
    else
        m_pumps.side_1 = SBT_e_pump_mode::PUMP_OFF;
int* data = m_pumps.data();
    m_can->send_frame(can_msg_id_t::PUMP_MODE_ID, 4, data);
delete[] data;
}

void Gui::on_sbSide_2_clicked(bool checked)
{
    ui->tb_logs->append(QString("on_sbSide_2_clicked() -> ") + (checked? "ON":"OFF"));
    if (checked)
        m_pumps.side_2 = SBT_e_pump_mode::PUMP_ON;
    else
        m_pumps.side_2 = SBT_e_pump_mode::PUMP_OFF;
int* data = m_pumps.data();
    m_can->send_frame(can_msg_id_t::PUMP_MODE_ID, 4, data);
delete[] data;
}
