/*! @file jpeg_rtsp_player.hpp
 *
 */

#ifndef RTSP_JPEG_RTSP_PLAYER_HPP
#define RTSP_JPEG_RTSP_PLAYER_HPP

#include <QDockWidget>
#include <QImage>

namespace rtsp_player {

class jpeg_player : public QWidget {
Q_OBJECT

public:
    jpeg_player();

    ~jpeg_player();


};

}


#include "jpeg_rtsp_player.moc"

#endif //RTSP_JPEG_RTSP_PLAYER_HPP
