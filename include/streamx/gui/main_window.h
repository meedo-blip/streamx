#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <memory>
#include "streamx/streaming_controller.h"

class StreamControlWidget;
class MonitoringWidget;
class PlatformsWidget;
class SettingsWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void UpdateStatusBar();
    void OnStreamStarted();
    void OnStreamStopped();
    void OnError(const QString& error);

private:
    void CreateUI();
    void CreateMenuBar();
    void CreateStatusBar();
    void ConnectSignals();
    void ApplyStylesheet();
    void StartUpdateTimer();
    void LoadPlatformsFromConfig();

    QTabWidget* tab_widget_;
    StreamControlWidget* stream_control_;
    MonitoringWidget* monitoring_;
    PlatformsWidget* platforms_;
    SettingsWidget* settings_;

    QLabel* status_label_;
    QLabel* connection_status_;
    QLabel* bitrate_label_;

    std::unique_ptr<streamx::StreamingController> controller_;
};
