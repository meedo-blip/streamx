#include "streamx/gui/stream_control_widget.h"
#include "streamx/utils/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>

StreamControlWidget::StreamControlWidget(streamx::StreamingController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller) {
    CreateUI();
    ConnectSignals();
}

void StreamControlWidget::CreateUI() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    // Capture Settings Group
    QGroupBox* capture_group = new QGroupBox("Capture Settings", this);
    QVBoxLayout* capture_layout = new QVBoxLayout(capture_group);

    QHBoxLayout* capture_row = new QHBoxLayout();
    capture_row->addWidget(new QLabel("Capture Type:"));
    capture_type_selector_ = new QComboBox();
    capture_type_selector_->addItem("Desktop");
    capture_type_selector_->addItem("Window");
    capture_type_selector_->addItem("Audio");
    capture_row->addWidget(capture_type_selector_);
    capture_row->addStretch();
    capture_layout->addLayout(capture_row);
    main_layout->addWidget(capture_group);

    // Stream Control Group
    QGroupBox* control_group = new QGroupBox("Stream Control", this);
    QVBoxLayout* control_layout = new QVBoxLayout(control_group);

    // Start/Stop buttons
    QHBoxLayout* button_row = new QHBoxLayout();
    start_button_ = new QPushButton("▶ Start Streaming");
    start_button_->setMinimumHeight(40);
    start_button_->setStyleSheet("background-color: #4caf50; font-weight: bold;");
    
    stop_button_ = new QPushButton("⏹ Stop Streaming");
    stop_button_->setMinimumHeight(40);
    stop_button_->setEnabled(false);
    stop_button_->setStyleSheet("background-color: #f44336; font-weight: bold;");

    button_row->addWidget(start_button_);
    button_row->addWidget(stop_button_);
    control_layout->addLayout(button_row);

    // Status label
    status_label_ = new QLabel("Status: Ready");
    status_label_->setStyleSheet("font-weight: bold; color: #4caf50;");
    control_layout->addWidget(status_label_);

    // CPU/GPU usage
    QHBoxLayout* usage_row = new QHBoxLayout();
    usage_row->addWidget(new QLabel("CPU Usage:"));
    cpu_usage_bar_ = new QProgressBar();
    cpu_usage_bar_->setMaximumWidth(200);
    usage_row->addWidget(cpu_usage_bar_);

    usage_row->addWidget(new QLabel("GPU Usage:"));
    gpu_usage_bar_ = new QProgressBar();
    gpu_usage_bar_->setMaximumWidth(200);
    usage_row->addWidget(gpu_usage_bar_);
    usage_row->addStretch();
    control_layout->addLayout(usage_row);

    main_layout->addWidget(control_group);

    // Platform Management Group
    QGroupBox* platform_group = new QGroupBox("Platform Management", this);
    QVBoxLayout* platform_layout = new QVBoxLayout(platform_group);

    QHBoxLayout* platform_row = new QHBoxLayout();
    platform_row->addWidget(new QLabel("Connected Platforms:"));
    platform_selector_ = new QComboBox();
    platform_row->addWidget(platform_selector_);
    
    add_platform_button_ = new QPushButton("➕ Add Platform");
    remove_platform_button_ = new QPushButton("❌ Remove");
    platform_row->addWidget(add_platform_button_);
    platform_row->addWidget(remove_platform_button_);
    platform_row->addStretch();
    platform_layout->addLayout(platform_row);

    main_layout->addWidget(platform_group);
    main_layout->addStretch();

    setLayout(main_layout);
}

void StreamControlWidget::ConnectSignals() {
    connect(start_button_, &QPushButton::clicked, this, &StreamControlWidget::OnStartClicked);
    connect(stop_button_, &QPushButton::clicked, this, &StreamControlWidget::OnStopClicked);
    connect(add_platform_button_, &QPushButton::clicked, this, &StreamControlWidget::OnAddPlatformClicked);
    connect(remove_platform_button_, &QPushButton::clicked, this, &StreamControlWidget::OnRemovePlatformClicked);
}

void StreamControlWidget::OnStartClicked() {
    if (!controller_) return;

    try {
        int display_index = 0;

        if (!controller_->StartCapture(display_index)) {
            emit Error("Failed to start capture");
            return;
        }

        if (!controller_->StartStreaming()) {
            emit Error("Failed to start streaming");
            return;
        }

        status_label_->setText("Status: Streaming ✓");
        status_label_->setStyleSheet("font-weight: bold; color: #2196f3;");
        start_button_->setEnabled(false);
        stop_button_->setEnabled(true);

        emit StreamStarted();
        STREAMX_INFO("Streaming started from GUI");
    } catch (const std::exception& e) {
        emit Error(QString::fromStdString(e.what()));
    }
}

void StreamControlWidget::OnStopClicked() {
    if (!controller_) return;

    try {
        controller_->StopStreaming();

        status_label_->setText("Status: Ready");
        status_label_->setStyleSheet("font-weight: bold; color: #4caf50;");
        start_button_->setEnabled(true);
        stop_button_->setEnabled(false);

        emit StreamStopped();
        STREAMX_INFO("Streaming stopped from GUI");
    } catch (const std::exception& e) {
        emit Error(QString::fromStdString(e.what()));
    }
}

void StreamControlWidget::OnAddPlatformClicked() {
    // Platform selection dialog
    QStringList platforms = {"Twitch", "YouTube", "StreamLabs", "Custom RTMP"};
    bool ok;
    QString platform = QInputDialog::getItem(this, "Add Platform", "Select platform:",
                                            platforms, 0, false, &ok);
    if (ok && !platform.isEmpty()) {
        QString key = QInputDialog::getText(this, "Stream Key", "Enter your stream key:",
                                           QLineEdit::Password, "", &ok);
        if (ok && !key.isEmpty()) {
            std::string platform_lower = platform.toLower().toStdString();
            std::string server_url;
            if (platform == "Custom RTMP") {
                QString server = QInputDialog::getText(this, "RTMP Server URL",
                                                      "Enter the RTMP server URL:",
                                                      QLineEdit::Normal, "", &ok);
                if (!ok || server.isEmpty()) {
                    return;
                }
                server_url = server.toStdString();
            }

            if (controller_->AddPlatform(platform_lower, platform_lower,
                                         key.toStdString(), server_url)) {
                platform_selector_->addItem(platform);
                STREAMX_INFO("Platform added: " + platform.toStdString());
            } else {
                QMessageBox::warning(this, "Error", "Failed to add platform");
            }
        }
    }
}

void StreamControlWidget::OnRemovePlatformClicked() {
    if (platform_selector_->count() == 0) {
        QMessageBox::warning(this, "No Platform", "No platform selected");
        return;
    }

    int index = platform_selector_->currentIndex();
    QString platform = platform_selector_->itemText(index);
    if (controller_->RemovePlatform(platform.toStdString())) {
        platform_selector_->removeItem(index);
        STREAMX_INFO("Platform removed: " + platform.toStdString());
    }
}

void StreamControlWidget::UpdateControlState() {
    bool is_streaming = controller_ && controller_->IsStreaming();
    start_button_->setEnabled(!is_streaming);
    stop_button_->setEnabled(is_streaming);
}
