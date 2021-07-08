#include "gui.hpp"


#include <QFile>
#include <QDebug>

#include <QJsonArray>
#include <QJsonDocument>

#include <QString>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include "can/sbt_can_msg.h"
#include "can/utils.h"
#include "epaper/Fonts/fonts.h"
#include "mav.hpp"
#include "mavlink/standard/mavlink.h"

extern "C"{
    #include "epaper/e-Paper/EPD_IT8951.h"
    #include "epaper/GUI/GUI_Paint.h"
    #include "epaper/GUI/GUI_BMPfile.h"
    #include "epaper/Config/Debug.h"
    #include "epaper/Config/DEV_Config.h"
}


static const double PI = 3.14159265359;
static const double DEG2RAD = (180.0 / PI);

UBYTE *Refresh_Frame_Buf = NULL;

UBYTE *Panel_Frame_Buf = NULL;
UBYTE *Panel_Area_Frame_Buf = NULL;

bool Four_Byte_Align = false;

extern int epd_mode;
extern UWORD VCOM;
extern UBYTE isColor;


Gui::Gui()
    :gps(new Gps(GPS_PORT)),
     m_can(new CAN(CAN_ITF)),
     m_mav("10.0.0.1")
{
    m_start_time = std::chrono::high_resolution_clock::now();

    m_can->start_receiving();
    connect(m_can, &CAN::frame_received, this, &Gui::on_can_frame_received);

    connect(gps, &Gps::position_updated, this, &Gui::on_gps_data_received);
    connect(gps, &Gps::fixChanged, this, &Gui::on_gps_fix_changed);

    InitGui();
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
    Paint_DrawNum(165, 100, speed_int, &Font24, 0x00, 0xFF); 
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
    //TODO: gotta think about this
  /*  this->ui->lGpsFix->setText(txt);
    if (is_fixed)
        this->ui->signal_icon->setStyleSheet("border-image:url(:gps/fix.png) 2 5 2 5;");
    else
        this->ui->signal_icon->setStyleSheet("border-image:url(:gps/no_fix.png) 2 5 2 5;"); */
}

void Gui::on_can_frame_received(can_frame cf)
{
    char frame_buff[100];
    sprint_canframe(frame_buff, &cf, ' ');
    QString frame_str(frame_buff);
    // tmp log

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
        printf("Unknown message id");
        return;
    }

    const QString id_string = id_names[can_ID - CAN_ID_OFFSET];
    const QString log_data = date + QString(" [%1]").arg(can_ID, 2) + " - " + id_string;

// to do : odczytanie parametrów
    switch (can_ID) {
    case can_msg_id_t::COOLING_SYSTEM_PARAMETERS_ID:
    {
        can_pump_state_input psi;
        memcpy(&psi, cf.data, sizeof(can_pump_state_input));
        double mot1_temp = psi.motor_one_temp;
        // double mot2_temp = psi.motor_two_temp;
     //  print temp here ui->pb_temp_1->setFormat(QString::number(mot1_temp) + " C");

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
        // ui->tb_errors->append(date + " " + id_string + " -> " + alarm_e[cf.data[0]] + "!");
        break;
    case can_msg_id_t::MOTOR_POWER_ID:
    {
        can_motor_power mp;
        memcpy(&mp, cf.data, sizeof(can_motor_power));
        double throttle = mp.potentiometer_value1/20.f + 50;
        // print throttle    ui->pb_poten1->setValue(throttle);
        break;
    }
    case can_msg_id_t::HYDROFOIL_ANGLE_ID:
        can_hydrofoil_angle ha;
        memcpy(&ha, cf.data, sizeof(can_hydrofoil_angle));
        break;

    case can_msg_id_t::BILGE_SYSTEM_ERRORS_ID:
        // ...
        // Coś z 'SBT_e_bilge_event' ?
        // ui->tb_errors->append(date + " " + id_string + " -> " /* + alarm_e[cf.data[0]] */ + "!");
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
	int char_power = (int)(dis_cur * (min_vol + max_vol)/2);

	Paint_DrawNum(500, 500, char_power, &Font24, 0x00, 0xFF); 

	// print vols and power here
	/* ui->lcd_max_vol->display(max_vol);
        ui->lcd_min_vol->display(min_vol);
        ui->lcd_max_curr->display(dis_cur);
        ui->lcd_min_curr->display(charge_cur);
        */ 
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
        // ui->tb_logs->append(QString::number(can_ID) + " - not defined");
        break;
    }



}

/******************************************************************************
function: Change direction of display, Called after Paint_NewImage()
parameter:
    mode: display mode
******************************************************************************/
static void Epd_Mode(int mode)
{
    if(mode == 3) {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_NONE);
        isColor = 1;
    }else if(mode == 2) {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_HORIZONTAL);
    }else if(mode == 1) {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_HORIZONTAL);
    }else {
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_NONE);
    }
}

/******************************************************************************
function: Display_InitGui
parameter:
    Panel_Width: Width of the panel
    Panel_Height: Height of the panel
    Init_Target_Memory_Addr: Memory address of IT8951 target memory address
    BitsPerPixel: Bits Per Pixel, 2^BitsPerPixel = grayscale
******************************************************************************/
UBYTE Display_InitGui(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr, UBYTE BitsPerPixel){
    const int epd_mode = 0;
    
    UWORD Display_Area_Width;
    if(Four_Byte_Align == true){
        Display_Area_Width = Panel_Width - (Panel_Width % 32);
    }else{
        Display_Area_Width = Panel_Width;
    }
    UWORD Display_Area_Height = Panel_Height;

    UWORD Display_Area_Sub_Width = Display_Area_Width / 5;
    UWORD Display_Area_Sub_Height = Display_Area_Height / 5;

    UDOUBLE Imagesize;

    Imagesize = ((Display_Area_Width * BitsPerPixel % 8 == 0)? (Display_Area_Width * BitsPerPixel / 8 ): (Display_Area_Width * BitsPerPixel / 8 + 1)) * Display_Area_Height;
    if((Refresh_Frame_Buf = (UBYTE *)malloc(Imagesize)) == NULL) {
        Debug("Failed to apply for image memory...\r\n");
        return -1;
    }

    Paint_NewImage(Refresh_Frame_Buf, Display_Area_Width, Display_Area_Height, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
      Epd_Mode(epd_mode);
    Paint_SetBitsPerPixel(BitsPerPixel);
    Paint_Clear(WHITE);

    /* ---- Actually print GUI ---- */
    
    // SPEED
    Paint_DrawRectangle(10, 10, 410, 205, 0x00, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(165, 25, "SPEED", &Font24, 0x00, 0xFF);



    //DISCHARGE
    Paint_DrawRectangle(525, 10, 925, 205, 0x00, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(650, 25, "DISCHARGE", &Font24, 0x00, 0xFF);



    //CHARGE
    Paint_DrawRectangle(1038, 10, 1438, 205, 0x00, DOT_PIXEL_3X3, DRAW_FILL_EMPTY); 
    Paint_DrawString_EN(1182, 25, "CHARGE", &Font24, 0x00, 0xFF); 
    

    // ***************************************************************************

    switch(BitsPerPixel){
        case BitsPerPixel_8:{
            EPD_IT8951_8bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width,  Display_Area_Height, false, Init_Target_Memory_Addr);
            break;
        }
        case BitsPerPixel_4:{
            EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, 0, Display_Area_Width,  Display_Area_Height, false, Init_Target_Memory_Addr,false);
            break;
        }
    }

    if(Refresh_Frame_Buf != NULL){
        free(Refresh_Frame_Buf);
        Refresh_Frame_Buf = NULL;
    }

    return 0;
}

void InitGui(){  
    const UWORD VCOM = 2100;
    const int epd_mode = 0;

    A2_Mode = 6;
    Four_Byte_Align = true;

    IT8951_Dev_Info Dev_Info;
    UWORD Panel_Width;
    UWORD Panel_Height;
    UDOUBLE Init_Target_Memory_Addr;
    	
    //Init the BCM2835 Device
    if(DEV_Module_Init()!=0){
        printf("ERROR");
        return;
    }

    Dev_Info = EPD_IT8951_Init(VCOM);

    Panel_Width = Dev_Info.Panel_W;
    Panel_Height = Dev_Info.Panel_H;
    Init_Target_Memory_Addr = Dev_Info.Memory_Addr_L | (Dev_Info.Memory_Addr_H << 16);

    Debug("VCOM value:%d\r\n", VCOM);
    Debug("Display mode:%d\r\n", epd_mode);
    Debug("A2 Mode:%d\r\n", A2_Mode);


    EPD_IT8951_Clear_Refresh(Dev_Info, Init_Target_Memory_Addr, INIT_Mode);
    Display_InitGui(Panel_Width, Panel_Height, Init_Target_Memory_Addr, BitsPerPixel_8);
    return;
}
