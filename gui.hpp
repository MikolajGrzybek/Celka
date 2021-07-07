#ifndef GUI_HPP
#define GUI_HPP

constexpr char GPS_PORT[] = "/dev/ttyACM0";
constexpr char CAN_ITF[]  = "can0";

#include <QWidget>
#include <QString>
#include <QJsonObject>

#include <chrono>

//QT_FORWARD_DECLARE_CLASS(QJsonArray)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QLabel)

#include "gps.hpp"
#include "can.hpp"
#include "mav.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class Gui; }
QT_END_NAMESPACE

// custom
typedef struct {
//    // 0   - zero
//    // 1,2 - poboczne
//    // 3   - cooling
//    // mode id < heade pomps[] ->payload // 11bit
//    uint8_t pomps[4];
//    pomps[0] =1;
    int8_t main_p;
    int8_t side_1;
    int8_t side_2;
    int8_t cooling;
    int* dt;
    int* data() {
        dt = new int[4];
        dt[0] = main_p;
        dt[1] = side_1;
        dt[2] = side_2;
        dt[3] = cooling;

        return dt;
        }
} SBT_pumps;

class Gui : public QWidget
{
    Q_OBJECT

private:
    Gps     *gps;
    CAN     *m_can;
    Ui::Gui *ui;
    Mavlink m_mav;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start_time;

    QList<QPushButton*> m_other_pomps_list;
    QList<QLabel*>      m_parameters_list;
    QJsonObject         m_parameters;

    SBT_pumps           m_pumps;

    void update_ui();

public:
    Gui(QWidget *parent = nullptr);
    ~Gui();

private slots:
    /* PAGE MANGMENT */
    void on_bHome_clicked();

    void on_bPage2_clicked();

    void on_bSettings_clicked();

    /* POMPS STEERING */
    void on_sbPomps_clicked();

    void on_sbCooling_clicked(bool checked);

    void on_sbSide_1_clicked(bool checked);

    void on_sbSide_2_clicked(bool checked);

    /* CALLBACKS */
    void on_gps_data_received(sbt_RMC_msg msg);

    void on_gps_fix_changed(bool is_fixed);

    void on_can_frame_received(can_frame cf);

    /* SETTINGS */
    bool load_settings();

    bool load_parameters();

    bool save_parameters() const;

    uint32_t get_program_ticks() {
        auto current_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(current_time - m_start_time).count();
    }


};
#endif // GUI_HPP
