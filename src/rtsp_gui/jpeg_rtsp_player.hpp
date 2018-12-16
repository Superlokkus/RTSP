/*! @file jpeg_rtsp_player.hpp
 *
 */

#ifndef RTSP_JPEG_RTSP_PLAYER_HPP
#define RTSP_JPEG_RTSP_PLAYER_HPP

#include <QWidget>
#include <QDockWidget>
#include <QStatusBar>

#include <memory>

namespace rtsp_player {

class jpeg_player : public QWidget {
Q_OBJECT

public:
    jpeg_player();
    ~jpeg_player();

    QDockWidget *get_control_widget();

    QDockWidget *get_settings_widget();

    QDockWidget *get_log_widget();

    void set_status_bar(QStatusBar *);

private:
    struct control_widget;
    struct settings_widget;
    struct status_widget;
    struct log_widget;
    struct impl;
    std::unique_ptr<impl> pimpl;

private slots:

    void setup();

    void play();

    void pause();

    void teardown();

    void option();

    void describe();

};

}


#endif //RTSP_JPEG_RTSP_PLAYER_HPP
