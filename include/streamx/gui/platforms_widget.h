#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include "streamx/streaming_controller.h"

class PlatformsWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlatformsWidget(streamx::StreamingController* controller, QWidget* parent = nullptr);
    void RefreshPlatformsList();

private slots:
    void OnAddPlatformClicked();
    void OnRemovePlatformClicked();
    void OnEditClicked();

private:
    void CreateUI();
    void SetupTable();
    void ConnectSignals();
    void ShowAddPlatformDialog();

    streamx::StreamingController* controller_;

    // UI Components
    QTableWidget* platforms_table_;
    QPushButton* add_button_;
    QPushButton* remove_button_;
    QPushButton* edit_button_;
    QLineEdit* stream_key_input_;
};
