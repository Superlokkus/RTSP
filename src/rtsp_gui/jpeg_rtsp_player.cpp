/*! @file jpeg_rtsp_player.cpp
 *
 */

#include "jpeg_rtsp_player.hpp"

#include <QLabel>
#include <QGridLayout>
#include <QBoxLayout>
#include <QImage>
#include <QPushButton>

#include <boost/log/trivial.hpp>
#include <QtGui/QtGui>


struct rtsp_player::jpeg_player::control_widget : QDockWidget {
    control_widget(QWidget *parent) :
            QDockWidget(QString::fromStdWString(L"Client Control"), parent) {
        this->setFeatures(DockWidgetFloatable | DockWidgetMovable);
        auto inside_widget = new QWidget();
        inside_widget->setLayout(new QBoxLayout(QBoxLayout::Direction::TopToBottom));

        auto button_widget = new QWidget();
        button_widget->setLayout(new QBoxLayout(QBoxLayout::LeftToRight));

        button_widget->layout()->addWidget(new QPushButton(QString::fromStdWString(L"Setup")));
        button_widget->layout()->addWidget(new QPushButton(QString::fromStdWString(L"Play")));
        button_widget->layout()->addWidget(new QPushButton(QString::fromStdWString(L"Pause")));
        button_widget->layout()->addWidget(new QPushButton(QString::fromStdWString(L"Teardown")));
        button_widget->layout()->addWidget(new QPushButton(QString::fromStdWString(L"Option")));


        inside_widget->layout()->addWidget(button_widget);
        this->setWidget(inside_widget);

    }

private:
Q_OBJECT
};

struct rtsp_player::jpeg_player::status_widget : QWidget {
    status_widget(QWidget *parent) : QWidget(parent) {}

private:
Q_OBJECT
};

struct rtsp_player::jpeg_player::impl {
    impl(QWidget *parent) :
            control_widget_(parent), status_widget_(parent) {

    }
    control_widget control_widget_;
    status_widget status_widget_;

    QStatusBar *status_bar_ = nullptr;
};

rtsp_player::jpeg_player::jpeg_player() : QWidget(), pimpl(std::make_unique<impl>(this)) {
    this->setLayout(new QGridLayout(this));
    auto image = QImage{"/Users/markus/Desktop/Untitled.jpeg"};
    auto label = new QLabel(this);
    this->layout()->addWidget(label);
    label->setPixmap(QPixmap::fromImage(image));

}

rtsp_player::jpeg_player::~jpeg_player() {
    BOOST_LOG_TRIVIAL(debug) << "To be ~destroyed";
}

QDockWidget *rtsp_player::jpeg_player::get_control_widget() {
    return &this->pimpl->control_widget_;
}

void rtsp_player::jpeg_player::set_status_bar(QStatusBar *status_bar) {
    this->pimpl->status_bar_ = status_bar;
}

#include "jpeg_rtsp_player.moc"
#include "moc_jpeg_rtsp_player.cpp"
