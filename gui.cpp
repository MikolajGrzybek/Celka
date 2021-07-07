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

    // Actually print GUI
    Paint_DrawRectangle(0, 0, Display_Area_Sub_Width, Display_Area_Sub_Height, 0x00, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
    Paint_DrawNum(Display_Area_Sub_Width*3/10, Display_Area_Sub_Height*1/4, 2137, &Font16, 0x20, 0xE0);
    Paint_DrawString_EN(Display_Area_Sub_Width*3/10, Display_Area_Sub_Height*3/4, "hello solarku", &Font16, 0x30, 0xD0);
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
    Display_InitGui(Panel_Width, Panel_Height, Init_Target_Memory_Addr, BitsPerPixel_4);
    return;
}