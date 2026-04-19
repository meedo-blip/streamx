#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include "streamx/streaming_controller.h"

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(streamx::StreamingController* controller, QWidget* parent = nullptr);

private slots:
    void OnSaveSettingsClicked();
    void OnLoadSettingsClicked();
    void OnResetToDefaultsClicked();
    void OnBitrateChanged(int value);
    void UpdateBitrateLabel(int value);

private:
    void CreateUI();
    void LoadCurrentSettings();
    void ConnectSignals();
    void ApplySettings();

    streamx::StreamingController* controller_;

    // Video Settings
    QSpinBox* width_spinbox_;
    QSpinBox* height_spinbox_;
    QSpinBox* fps_spinbox_;
    QSlider* bitrate_slider_;
    QComboBox* codec_selector_;
    QComboBox* preset_selector_;
    QCheckBox* enable_bframes_;
    QSpinBox* gop_size_spinbox_;

    // Audio Settings
    QSpinBox* sample_rate_spinbox_;
    QSpinBox* channels_spinbox_;
    QSpinBox* audio_bitrate_spinbox_;
    QComboBox* audio_codec_selector_;

    // Encoding Settings
    QCheckBox* gpu_encoding_;
    QCheckBox* adaptive_bitrate_;

    // Buttons
    QPushButton* save_button_;
    QPushButton* load_button_;
    QPushButton* reset_button_;
};
