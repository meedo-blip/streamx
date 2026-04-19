#include "streamx/gui/main_window.h"
#include "streamx/gui/stream_control_widget.h"
#include "streamx/gui/monitoring_widget.h"
#include "streamx/gui/platforms_widget.h"
#include "streamx/gui/settings_widget.h"
#include "streamx/utils/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QCloseEvent>
#include <QKeySequence>
#include <QAction>
#include <QStatusBar>
#include <QTimer>
#include <QApplication>
#include <QStyle>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      controller_(std::make_unique<streamx::StreamingController>()) {
    
    setWindowTitle("StreamX - Multi-Platform Streaming");
    setWindowIcon(QIcon(":/icons/streamx.png"));
    setGeometry(100, 100, 1200, 800);

    // Initialize controller
    streamx::VideoConfig video{
        .width = 1920, .height = 1080, .fps = 60,
        .bitrate_kbps = 5000, .codec = streamx::CodecType::H264
    };
    streamx::AudioConfig audio{
        .sample_rate = 48000, .channels = 2, .bitrate_kbps = 128
    };
    controller_->Initialize(video, audio);

    CreateUI();
    CreateMenuBar();
    CreateStatusBar();
    ConnectSignals();
    ApplyStylesheet();
    StartUpdateTimer();
}

MainWindow::~MainWindow() {
}

void MainWindow::CreateUI() {
    // Create tab widget
    tab_widget_ = new QTabWidget(this);

    // Create widgets for each tab
    stream_control_ = new StreamControlWidget(controller_.get(), this);
    monitoring_ = new MonitoringWidget(controller_.get(), this);
    platforms_ = new PlatformsWidget(controller_.get(), this);
    settings_ = new SettingsWidget(controller_.get(), this);

    // Add tabs
    tab_widget_->addTab(stream_control_, "Stream Control");
    tab_widget_->addTab(monitoring_, "Monitoring");
    tab_widget_->addTab(platforms_, "Platforms");
    tab_widget_->addTab(settings_, "Settings");

    setCentralWidget(tab_widget_);
}

void MainWindow::CreateMenuBar() {
    QMenuBar* menubar = menuBar();

    // File menu
    QMenu* file_menu = menubar->addMenu("&File");
    
    QAction* exit_action = file_menu->addAction("E&xit");
    exit_action->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(exit_action, &QAction::triggered, this, &QApplication::quit);

    // View menu
    QMenu* view_menu = menubar->addMenu("&View");
    view_menu->addAction("Refresh")->setShortcut(Qt::Key_F5);

    // Help menu
    QMenu* help_menu = menubar->addMenu("&Help");
    QAction* about_action = help_menu->addAction("&About StreamX");
    connect(about_action, &QAction::triggered, [this]() {
        STREAMX_INFO("About StreamX clicked");
    });
}

void MainWindow::CreateStatusBar() {
    status_label_ = new QLabel("Ready", this);
    connection_status_ = new QLabel("Disconnected", this);
    bitrate_label_ = new QLabel("0 kbps", this);

    statusBar()->addWidget(status_label_);
    statusBar()->addPermanentWidget(bitrate_label_);
    statusBar()->addPermanentWidget(connection_status_);
}

void MainWindow::ConnectSignals() {
    connect(stream_control_, &StreamControlWidget::StreamStarted, this, &MainWindow::OnStreamStarted);
    connect(stream_control_, &StreamControlWidget::StreamStopped, this, &MainWindow::OnStreamStopped);
    connect(stream_control_, QOverload<const QString&>::of(&StreamControlWidget::Error), 
            this, &MainWindow::OnError);
}

void MainWindow::ApplyStylesheet() {
    // Modern dark stylesheet
    QString stylesheet = R"(
        QMainWindow {
            background-color: #2b2b2b;
            color: #ffffff;
        }
        QTabWidget::pane {
            border: 1px solid #3d3d3d;
        }
        QTabBar::tab {
            background-color: #3d3d3d;
            color: #ffffff;
            padding: 5px 15px;
            margin-right: 2px;
            border: 1px solid #4d4d4d;
        }
        QTabBar::tab:selected {
            background-color: #0d47a1;
            color: #ffffff;
        }
        QPushButton {
            background-color: #0d47a1;
            color: #ffffff;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1565c0;
        }
        QPushButton:pressed {
            background-color: #0c3a86;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #3d3d3d;
            color: #ffffff;
            border: 1px solid #4d4d4d;
            padding: 5px;
            border-radius: 3px;
        }
        QTableWidget {
            background-color: #2b2b2b;
            color: #ffffff;
            gridline-color: #3d3d3d;
        }
        QHeaderView::section {
            background-color: #0d47a1;
            color: #ffffff;
            padding: 5px;
            border: none;
        }
        QProgressBar {
            border: 1px solid #3d3d3d;
            border-radius: 3px;
            background-color: #1a1a1a;
            height: 20px;
        }
        QProgressBar::chunk {
            background-color: #0d47a1;
        }
        QLabel {
            color: #ffffff;
        }
    )";
    
    qApp->setStyle("Fusion");
    qApp->setStyleSheet(stylesheet);
}

void MainWindow::StartUpdateTimer() {
    QTimer* update_timer = new QTimer(this);
    connect(update_timer, &QTimer::timeout, this, &MainWindow::UpdateStatusBar);
    update_timer->start(1000);  // Update every second
}

void MainWindow::UpdateStatusBar() {
    if (controller_->IsStreaming()) {
        status_label_->setText("Live");
        auto platforms = controller_->GetConnectedPlatforms();
        if (platforms.empty()) {
            connection_status_->setText("Streaming via GPU Screen Recorder");
        } else {
            connection_status_->setText(QString::fromStdString(
                std::to_string(platforms.size()) + 
                " platform(s) connected"));
        }
    } else {
        status_label_->setText("Ready");
        connection_status_->setText("Disconnected");
    }
}

void MainWindow::OnStreamStarted() {
    STREAMX_INFO("Stream started from GUI");
    status_label_->setText("Streaming");
}

void MainWindow::OnStreamStopped() {
    STREAMX_INFO("Stream stopped from GUI");
    status_label_->setText("Ready");
}

void MainWindow::OnError(const QString& error) {
    STREAMX_ERROR("GUI Error: " + error.toStdString());
    status_label_->setText("Error: " + error);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (controller_->IsStreaming()) {
        controller_->StopStreaming();
    }
    event->accept();
}
