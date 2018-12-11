/*! @file jpeg_rtsp_player.cpp
 *
 */

#include "jpeg_rtsp_player.hpp"

#include <QLabel>
#include <QGridLayout>

#include <boost/log/trivial.hpp>

rtsp_player::jpeg_player::jpeg_player() : QWidget() {
    this->setLayout(new QGridLayout(this));
    auto image = QImage{"/Users/markus/Desktop/Untitled.jpeg"};
    auto label = new QLabel(this);
    this->layout()->addWidget(label);
    label->setPixmap(QPixmap::fromImage(image));

}

rtsp_player::jpeg_player::~jpeg_player() {
    BOOST_LOG_TRIVIAL(debug) << "To be ~destroyed";
}

