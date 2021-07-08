#ifndef GUI_HPP
#define GUI_HPP

constexpr char GPS_PORT[] = "/dev/ttyACM0";
constexpr char CAN_ITF[]  = "can0";

#include <QString>
#include <QJsonObject>

#include <chrono>

//QT_FORWARD_DECLARE_CLASS(QJsonArray)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)

#include "gps.hpp"
#include "can.hpp"
#include "mav.hpp"

extern "C"{
    #include "epaper/e-Paper/EPD_IT8951.h"
    #include "epaper/Config/DEV_Config.h"
}


QT_BEGIN_NAMESPACE
namespace Ui { class Gui; }
QT_END_NAMESPACE


// 4 bit per pixel, which is 16 grayscale
#define BitsPerPixel_4 4
// 8 bit per pixel, which is 256 grayscale, but will automatically reduce by hardware to 4bpp, which is 16 grayscale
#define BitsPerPixel_8 8


//For all refresh fram buf except touch panel
extern UBYTE *Refresh_Frame_Buf;

extern bool Four_Byte_Align;


typedef struct{
    UWORD Dynamic_Area_Width;
    UWORD Dynamic_Area_Height;

    UDOUBLE Imagesize;

    UWORD Start_X;
    UWORD Start_Y;

    UWORD Dynamic_Area_Count;
    UWORD Repeat_Area_Times;

    clock_t Dynamic_Area_Start;
    clock_t Dynamic_Area_Finish;
    double Dynamic_Area_Duration; 
} Dynamic_Ref_Area;


class Gui : public QObject
{
    Q_OBJECT
private:
    Gps     *gps;
    CAN     *m_can;
    Mavlink m_mav;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start_time;

    QList<QPushButton*> m_other_pomps_list;
    QList<QLabel*>      m_parameters_list;
    QJsonObject         m_parameters;

public:
    Gui();

private slots:
    void on_gps_data_received(sbt_RMC_msg msg);

    void on_gps_fix_changed(bool is_fixed);

    void on_can_frame_received(can_frame cf);

    uint32_t get_program_ticks() {
        auto current_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(current_time - m_start_time).count();
    }


};

/* GUI */
UBYTE Display_InitGui(UWORD Panel_Width, UWORD Panel_Height, UDOUBLE Init_Target_Memory_Addr, UBYTE BitsPerPixel);

void InitGui();

UBYTE Dynamic_Refresh(UWORD width, UWORD height, UWORD start_x, UWORD start_y);


#endif // GUI_HPP
