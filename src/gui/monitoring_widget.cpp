#include "streamx/gui/monitoring_widget.h"
#include "streamx/utils/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <iomanip>
#include <sstream>

MonitoringWidget::MonitoringWidget(streamx::StreamingController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller), update_timer_(new QTimer(this)) {
    CreateUI();
    SetupTable();
    ConnectSignals();
}

void MonitoringWidget::CreateUI() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    // Overall Health Group
    QGroupBox* health_group = new QGroupBox("Overall Stream Health", this);
    QVBoxLayout* health_layout = new QVBoxLayout(health_group);

    // Overall status
    overall_health_label_ = new QLabel("Status: Disconnected", this);
    overall_health_label_->setStyleSheet("font-weight: bold; font-size: 12pt;");
    health_layout->addWidget(overall_health_label_);

    // Bitrate progress
    QHBoxLayout* bitrate_row = new QHBoxLayout();
    bitrate_row->addWidget(new QLabel("Overall Bitrate:"));
    overall_bitrate_bar_ = new QProgressBar();
    overall_bitrate_bar_->setMaximum(10000);  // 10 Mbps max
    bitrate_row->addWidget(overall_bitrate_bar_);
    health_layout->addLayout(bitrate_row);

    // Latency progress
    QHBoxLayout* latency_row = new QHBoxLayout();
    latency_row->addWidget(new QLabel("Avg Latency:"));
    avg_latency_bar_ = new QProgressBar();
    avg_latency_bar_->setMaximum(5000);  // 5 sec max
    latency_row->addWidget(avg_latency_bar_);
    health_layout->addLayout(latency_row);

    // Stats row
    QHBoxLayout* stats_row = new QHBoxLayout();
    total_frames_label_ = new QLabel("Frames: 0");
    uptime_label_ = new QLabel("Uptime: 0s");
    stats_row->addWidget(total_frames_label_);
    stats_row->addWidget(uptime_label_);
    stats_row->addStretch();
    health_layout->addLayout(stats_row);

    main_layout->addWidget(health_group);

    // Per-Platform Metrics Group
    QGroupBox* platforms_group = new QGroupBox("Per-Platform Metrics", this);
    QVBoxLayout* platforms_layout = new QVBoxLayout(platforms_group);

    platforms_table_ = new QTableWidget(this);
    platforms_layout->addWidget(platforms_table_);

    main_layout->addWidget(platforms_group);
}

void MonitoringWidget::SetupTable() {
    platforms_table_->setColumnCount(7);
    platforms_table_->setHorizontalHeaderLabels({
        "Platform", "Status", "Bitrate (kbps)", "Latency (ms)",
        "Frames Dropped", "Reconnects", "Last Update"
    });

    platforms_table_->horizontalHeader()->setStretchLastSection(true);
    platforms_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    platforms_table_->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MonitoringWidget::ConnectSignals() {
    connect(update_timer_, &QTimer::timeout, this, &MonitoringWidget::UpdateMetrics);
    connect(update_timer_, &QTimer::timeout, this, &MonitoringWidget::RefreshPlatformTable);
    update_timer_->start(500);  // Update every 500ms
}

void MonitoringWidget::UpdateMetrics() {
    if (!controller_) return;

    auto health = controller_->GetOverallHealth();

    // Update overall health
    if (health.connected) {
        overall_health_label_->setText("Status: 🟢 Connected");
        overall_health_label_->setStyleSheet("font-weight: bold; font-size: 12pt; color: #4caf50;");
    } else {
        overall_health_label_->setText("Status: 🔴 Disconnected");
        overall_health_label_->setStyleSheet("font-weight: bold; font-size: 12pt; color: #f44336;");
    }

    // Update bitrate bar
    int bitrate_value = static_cast<int>(health.bitrate_actual_kbps);
    overall_bitrate_bar_->setValue(bitrate_value);

    // Update latency bar
    int latency_value = static_cast<int>(health.network_latency_ms);
    avg_latency_bar_->setValue(latency_value);

    // Update stats labels
    total_frames_label_->setText(QString("Frames: %1").arg(0));  // Would get from controller
    uptime_label_->setText(QString("Latency: %1 ms").arg(static_cast<int>(health.network_latency_ms)));
}

void MonitoringWidget::RefreshPlatformTable() {
    if (!controller_) return;

    auto health_map = controller_->GetPlatformHealth();

    platforms_table_->setRowCount(health_map.size());

    int row = 0;
    for (const auto& [name, health] : health_map) {
        // Platform name
        auto name_item = new QTableWidgetItem(QString::fromStdString(name));
        platforms_table_->setItem(row, 0, name_item);

        // Status
        auto status_item = new QTableWidgetItem(health.connected ? "🟢 Connected" : "🔴 Disconnected");
        platforms_table_->setItem(row, 1, status_item);

        // Bitrate
        auto bitrate_item = new QTableWidgetItem(
            QString::number(health.bitrate_actual_kbps, 'f', 1));
        platforms_table_->setItem(row, 2, bitrate_item);

        // Latency
        auto latency_item = new QTableWidgetItem(
            QString::number(health.network_latency_ms, 'f', 1));
        platforms_table_->setItem(row, 3, latency_item);

        // Frames dropped
        auto dropped_item = new QTableWidgetItem(
            QString::number(health.frames_dropped));
        platforms_table_->setItem(row, 4, dropped_item);

        // Reconnects
        auto reconnects_item = new QTableWidgetItem(
            QString::number(health.reconnect_count));
        platforms_table_->setItem(row, 5, reconnects_item);

        // Last update (timestamp)
        auto last_update = new QTableWidgetItem("Now");
        platforms_table_->setItem(row, 6, last_update);

        row++;
    }
}
