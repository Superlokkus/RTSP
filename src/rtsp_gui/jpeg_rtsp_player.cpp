/*! @file jpeg_rtsp_player.cpp
 *
 */

#include "jpeg_rtsp_player.hpp"

#include <QLabel>
#include <QGridLayout>
#include <QBoxLayout>
#include <QFormLayout>
#include <QImage>
#include <QPushButton>
#include <QLineEdit>
#include <QtGui/QtGui>
#include <QTextEdit>

#include <boost/log/trivial.hpp>

#include <streaming_lib.hpp>


struct rtsp_player::jpeg_player::control_widget : QDockWidget {
    control_widget(jpeg_player *parent) :
            QDockWidget(QString::fromStdWString(L"Client Control")) {
        this->setFeatures(DockWidgetFloatable | DockWidgetMovable);
        auto inside_widget = new QWidget();
        inside_widget->setLayout(new QBoxLayout(QBoxLayout::Direction::TopToBottom));

        auto button_widget = new QWidget();
        button_widget->setLayout(new QBoxLayout(QBoxLayout::LeftToRight));

        auto setup_button = new QPushButton(QString::fromStdWString(L"Setup"));
        button_widget->layout()->addWidget(setup_button);
        connect(setup_button, SIGNAL(clicked()), parent, SLOT(setup()));
        auto play_button = new QPushButton(QString::fromStdWString(L"Play"));
        button_widget->layout()->addWidget(play_button);
        connect(play_button, SIGNAL(clicked()), parent, SLOT(play()));
        auto pause_button = new QPushButton(QString::fromStdWString(L"Pause"));
        button_widget->layout()->addWidget(pause_button);
        connect(pause_button, SIGNAL(clicked()), parent, SLOT(pause()));
        auto teardown_button = new QPushButton(QString::fromStdWString(L"Teardown"));
        button_widget->layout()->addWidget(teardown_button);
        connect(teardown_button, SIGNAL(clicked()), parent, SLOT(teardown()));
        auto option_button = new QPushButton(QString::fromStdWString(L"Option"));
        button_widget->layout()->addWidget(option_button);
        connect(option_button, SIGNAL(clicked()), parent, SLOT(option()));
        auto describe_button = new QPushButton(QString::fromStdWString(L"Describe"));
        button_widget->layout()->addWidget(describe_button);
        connect(describe_button, SIGNAL(clicked()), parent, SLOT(describe()));

        inside_widget->layout()->addWidget(button_widget);
        this->setWidget(inside_widget);

    }

private:
Q_OBJECT
};

struct rtsp_player::jpeg_player::settings_widget : QDockWidget {
    settings_widget() : QDockWidget(QString::fromStdWString(L"Client Settings")) {
        this->setFeatures(DockWidgetFloatable | DockWidgetMovable);
        auto inside_widget = new QWidget();
        auto form_layout = new QFormLayout();
        inside_widget->setLayout(form_layout);

        host_line_edit = new QLineEdit(QString::fromStdWString(L"rtsp://localhost:5054/movie.mjpeg"));
        host_line_edit->setMaxLength(1024);
        form_layout->addRow(tr("&Host:"), host_line_edit);

        this->setWidget(inside_widget);
    }

    std::string get_current_url() const {
        return this->host_line_edit->text().toStdString();
    }

private:
Q_OBJECT

    QLineEdit *host_line_edit = nullptr;
};

struct rtsp_player::jpeg_player::log_widget : QDockWidget {
    log_widget() : QDockWidget(QString::fromStdWString(L"Client Log")) {
        this->setFeatures(DockWidgetFloatable | DockWidgetMovable);
        text_widget_ = new QTextEdit();
        text_widget_->setReadOnly(true);

        this->setWidget(text_widget_);
    }

public slots:

    void add_log(const QString &log) {
        this->text_widget_->append(log);
    };

    void add_error_log(const QString &log) {
        this->text_widget_->append(log);
    };

private:
Q_OBJECT

    QTextEdit *text_widget_;
};

struct rtsp_player::jpeg_player::status_widget : QWidget {
    status_widget() : QWidget() {}

private:
Q_OBJECT
};

struct rtsp_player::jpeg_player::impl {
    impl(jpeg_player *parent) :
            control_widget_(parent), settings_widget_(), status_widget_(), log_widget_() {}

    control_widget control_widget_;
    settings_widget settings_widget_;
    status_widget status_widget_;
    log_widget log_widget_;

    QStatusBar *status_bar_ = nullptr;

    std::unique_ptr<rtsp::rtsp_client_pimpl> rtsp_client_;
};

rtsp_player::jpeg_player::jpeg_player() : QWidget(), pimpl(std::make_unique<impl>(this)) {
    this->setLayout(new QGridLayout(this));
    auto image = QImage{"/Users/markus/Desktop/Untitled.jpeg"};
    auto label = new QLabel(this);
    this->layout()->addWidget(label);
    label->setPixmap(QPixmap::fromImage(image));

}

rtsp_player::jpeg_player::~jpeg_player() {
    this->pimpl->rtsp_client_.reset();
    BOOST_LOG_TRIVIAL(debug) << "To be ~destroyed";
}

QDockWidget *rtsp_player::jpeg_player::get_control_widget() {
    return &this->pimpl->control_widget_;
}

QDockWidget *rtsp_player::jpeg_player::get_settings_widget() {
    return &this->pimpl->settings_widget_;
}

QDockWidget *rtsp_player::jpeg_player::get_log_widget() {
    return &this->pimpl->log_widget_;
}

void rtsp_player::jpeg_player::set_status_bar(QStatusBar *status_bar) {
    this->pimpl->status_bar_ = status_bar;
}

void rtsp_player::jpeg_player::setup() {
    this->pimpl->status_bar_->showMessage(QString::fromStdWString(L"Setup"), 1000);
    this->pimpl->rtsp_client_ = std::make_unique<rtsp::rtsp_client_pimpl>(
            this->pimpl->settings_widget_.get_current_url(),
            [this](auto exception) {
                QMetaObject::invokeMethod( //Could be UB see https://stackoverflow.com/q/53803018/3537677
                        this->get_log_widget(), "add_error_log", Qt::QueuedConnection,
                        Q_ARG(QString, QString{exception.what()})
                );
            },
            [this](auto log) {
                QMetaObject::invokeMethod(//Could be UB see https://stackoverflow.com/q/53803018/3537677
                        this->get_log_widget(), "add_log", Qt::QueuedConnection,
                        Q_ARG(QString, QString::fromStdString(log))
                );
            }
    );
}

void rtsp_player::jpeg_player::play() {
    this->pimpl->status_bar_->showMessage(QString::fromStdWString(L"Play"), 1000);
}

void rtsp_player::jpeg_player::pause() {
    this->pimpl->status_bar_->showMessage(QString::fromStdWString(L"Pause"), 1000);
}

void rtsp_player::jpeg_player::teardown() {
    this->pimpl->status_bar_->showMessage(QString::fromStdWString(L"Teardown"), 1000);
}

void rtsp_player::jpeg_player::option() {
    this->pimpl->status_bar_->showMessage(QString::fromStdWString(L"Option"), 1000);
}

void rtsp_player::jpeg_player::describe() {
    this->pimpl->status_bar_->showMessage(QString::fromStdWString(L"Describe"), 1000);
}

#include "jpeg_rtsp_player.moc"
#include "moc_jpeg_rtsp_player.cpp"
