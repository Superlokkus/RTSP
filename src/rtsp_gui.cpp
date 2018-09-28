/*! @file rtsp_gui.cpp
 *
 * */

#include <QtGui/QApplication>
#include <QtGui>
#include <QDockWidget>
#include <string>
#include <iostream>

#include <boost/log/trivial.hpp>


#include <streaming_lib.hpp>

class rtsp_gui_widget : public QMainWindow {
Q_OBJECT

public:
    enum struct application_mode {
        none,
        server,
        client,
    };

    rtsp_gui_widget(application_mode preselection = application_mode::none)
            : QMainWindow() {
        auto toolbar = new QToolBar;
        this->addToolBar(toolbar);
        auto server_action = toolbar->addAction(QString::fromStdWString(L"Server"));
        auto client_action = toolbar->addAction(QString::fromStdWString(L"Client"));
        connect(server_action, SIGNAL(triggered()), this, SLOT(show_server_page()));
        connect(client_action, SIGNAL(triggered()), this, SLOT(show_client_page()));

        this->setCentralWidget(nullptr);

        if (preselection == application_mode::client)
            show_client_page();
        else if (preselection == application_mode::server)
            show_server_page();
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

    auto preselection{rtsp_gui_widget::application_mode::none};
    if (argc > 1) {
        const std::string first_flag{argv[1]};
        if (first_flag == "server")
            preselection = rtsp_gui_widget::application_mode::server;
        else if (first_flag == "client")
            preselection = rtsp_gui_widget::application_mode::client;
        else
            std::cerr << "Usage: " << argv[0] << " <application mode>\n" <<
                      "<application mode> = \"server\" | \"client\"" << "\n";
    }
    auto window = new rtsp_gui_widget(preselection);

    window->show();

    return QApplication::exec();
}

#include "rtsp_gui.moc"