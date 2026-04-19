#pragma once

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include "streamx/streaming_controller.h"

class StreamControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit StreamControlWidget(streamx::StreamingController* controller, QWidget* parent = nullptr);

signals:
    void StreamStarted();
    void StreamStopped();
    void Error(const QString& message);

private slots:
    void OnStartClicked();
    void OnStopClicked();
    void OnAddPlatformClicked();
    void OnRemovePlatformClicked();
    void UpdateControlState();

private:
    void CreateUI();
    void ConnectSignals();

    streamx::StreamingController* controller_;

    // UI Components
    QPushButton* start_button_;
    QPushButton* stop_button_;
    QPushButton* add_platform_button_;
    QPushButton* remove_platform_button_;
    QComboBox* platform_selector_;
    QComboBox* capture_type_selector_;
    QLabel* status_label_;
    QProgressBar* cpu_usage_bar_;
    QProgressBar* gpu_usage_bar_;
};
