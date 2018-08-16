/*! @file rtsp_gui.cpp
 *
 * */

#include <QtGui/QApplication>
#include <QtGui>
#include <QDockWidget>
#include <string>

class rtsp_gui_widget : public QMainWindow {
Q_OBJECT

public:
    rtsp_gui_widget()
            : QMainWindow() {
        auto toolbar = new QToolBar;
        this->addToolBar(toolbar);

    }
};

int main(int argc, char *argv[]) {
    QApplication this_application(argc, argv);

    auto window = new rtsp_gui_widget;

    window->show();

    return QApplication::exec();
}

#include "rtsp_gui.moc"
