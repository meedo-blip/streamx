#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include "streamx/streaming_controller.h"

class MonitoringWidget : public QWidget {
    Q_OBJECT

public:
    explicit MonitoringWidget(streamx::StreamingController* controller, QWidget* parent = nullptr);

private slots:
    void UpdateMetrics();
    void RefreshPlatformTable();

private:
    void CreateUI();
    void SetupTable();
    void ConnectSignals();

    streamx::StreamingController* controller_;
    QTimer* update_timer_;

    // UI Components
    QProgressBar* overall_bitrate_bar_;
    QProgressBar* avg_latency_bar_;
    QTableWidget* platforms_table_;
    QLabel* total_frames_label_;
    QLabel* uptime_label_;
    QLabel* overall_health_label_;
};
