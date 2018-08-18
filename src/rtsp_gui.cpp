/*! @file rtsp_gui.cpp
 *
 * */

#include <QtGui/QApplication>
#include <QtGui>
#include <QDockWidget>
#include <string>

#include <boost/log/trivial.hpp>


#include <streaming_lib.hpp>

class rtsp_gui_widget : public QMainWindow {
Q_OBJECT

public:
    rtsp_gui_widget()
            : QMainWindow() {
        auto toolbar = new QToolBar;
        this->addToolBar(toolbar);
        auto server_action = toolbar->addAction(QString::fromStdWString(L"Server"));
        auto client_action = toolbar->addAction(QString::fromStdWString(L"Client"));
        connect(server_action, SIGNAL(triggered()), this, SLOT(show_server_page()));
        connect(client_action, SIGNAL(triggered()), this, SLOT(show_client_page()));

        this->setCentralWidget(nullptr);

    }

public slots:

    void show_server_page() {
        BOOST_LOG_TRIVIAL(debug) << "server";
    }

    void show_client_page() {
        BOOST_LOG_TRIVIAL(debug) << "client";
    }

};

int main(int argc, char *argv[]) {
    QApplication this_application(argc, argv);

    auto window = new rtsp_gui_widget;

    window->show();

    return QApplication::exec();
}

#include "rtsp_gui.moc"
