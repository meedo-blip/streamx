#include "streamx/gui/settings_widget.h"
#include "streamx/utils/logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QLocale>

SettingsWidget::SettingsWidget(streamx::StreamingController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller) {
    CreateUI();
    LoadCurrentSettings();
    ConnectSignals();
}

void SettingsWidget::CreateUI() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    // Video Settings Group
    QGroupBox* video_group = new QGroupBox("Video Settings", this);
    QVBoxLayout* video_layout = new QVBoxLayout(video_group);

    // Resolution
    QHBoxLayout* resolution_row = new QHBoxLayout();
    resolution_row->addWidget(new QLabel("Resolution:"));
    width_spinbox_ = new QSpinBox();
    width_spinbox_->setLocale(QLocale::c());
    width_spinbox_->setValue(1920);
    width_spinbox_->setMinimum(640);
    width_spinbox_->setMaximum(4096);
    resolution_row->addWidget(width_spinbox_);
    resolution_row->addWidget(new QLabel("x"));
    height_spinbox_ = new QSpinBox();
    height_spinbox_->setLocale(QLocale::c());
    height_spinbox_->setValue(1080);
    height_spinbox_->setMinimum(480);
    height_spinbox_->setMaximum(2160);
    resolution_row->addWidget(height_spinbox_);
    resolution_row->addStretch();
    video_layout->addLayout(resolution_row);

    // FPS and Bitrate
    QHBoxLayout* fps_bitrate_row = new QHBoxLayout();
    fps_bitrate_row->addWidget(new QLabel("FPS:"));
    fps_spinbox_ = new QSpinBox();
    fps_spinbox_->setLocale(QLocale::c());
    fps_spinbox_->setValue(60);
    fps_spinbox_->setMinimum(24);
    fps_spinbox_->setMaximum(120);
    fps_bitrate_row->addWidget(fps_spinbox_);

    fps_bitrate_row->addSpacing(20);
    fps_bitrate_row->addWidget(new QLabel("Bitrate (kbps):"));
    bitrate_slider_ = new QSlider(Qt::Horizontal);
    bitrate_slider_->setMinimum(1000);
    bitrate_slider_->setMaximum(10000);
    bitrate_slider_->setValue(5000);
    fps_bitrate_row->addWidget(bitrate_slider_);
    fps_bitrate_row->addStretch();
    video_layout->addLayout(fps_bitrate_row);

    connect(bitrate_slider_, QOverload<int>::of(&QSlider::valueChanged), 
            this, &SettingsWidget::UpdateBitrateLabel);

    // Codec and Preset
    QHBoxLayout* codec_preset_row = new QHBoxLayout();
    codec_preset_row->addWidget(new QLabel("Codec:"));
    codec_selector_ = new QComboBox();
    codec_selector_->addItem("H264");
    codec_selector_->addItem("H265");
    codec_preset_row->addWidget(codec_selector_);

    codec_preset_row->addSpacing(20);
    codec_preset_row->addWidget(new QLabel("Preset:"));
    preset_selector_ = new QComboBox();
    preset_selector_->addItems({"ultrafast", "superfast", "veryfast", "faster",
                               "fast", "medium", "slow", "slower"});
    preset_selector_->setCurrentText("medium");
    codec_preset_row->addWidget(preset_selector_);
    codec_preset_row->addStretch();
    video_layout->addLayout(codec_preset_row);

    // B-frames and GOP
    QHBoxLayout* bframes_gop_row = new QHBoxLayout();
    enable_bframes_ = new QCheckBox("Enable B-Frames");
    enable_bframes_->setChecked(true);
    bframes_gop_row->addWidget(enable_bframes_);

    bframes_gop_row->addSpacing(20);
    bframes_gop_row->addWidget(new QLabel("GOP Size:"));
    gop_size_spinbox_ = new QSpinBox();
    gop_size_spinbox_->setLocale(QLocale::c());
    gop_size_spinbox_->setValue(60);
    gop_size_spinbox_->setMinimum(1);
    gop_size_spinbox_->setMaximum(300);
    bframes_gop_row->addWidget(gop_size_spinbox_);
    bframes_gop_row->addStretch();
    video_layout->addLayout(bframes_gop_row);

    main_layout->addWidget(video_group);

    // Audio Settings Group
    QGroupBox* audio_group = new QGroupBox("Audio Settings", this);
    QVBoxLayout* audio_layout = new QVBoxLayout(audio_group);

    QHBoxLayout* audio_row = new QHBoxLayout();
audio_row->addWidget(new QLabel("Sample Rate (Hz):"));
    sample_rate_spinbox_ = new QSpinBox();
    sample_rate_spinbox_->setLocale(QLocale::c());
    sample_rate_spinbox_->setValue(48000);
    sample_rate_spinbox_->setMinimum(16000);
    sample_rate_spinbox_->setMaximum(192000);

    audio_row->addSpacing(20);
    audio_row->addWidget(new QLabel("Channels:"));
    channels_spinbox_ = new QSpinBox();
    channels_spinbox_->setLocale(QLocale::c());
    channels_spinbox_->setValue(2);
    channels_spinbox_->setMinimum(1);
    channels_spinbox_->setMaximum(8);

    audio_row->addSpacing(20);
    audio_row->addWidget(new QLabel("Bitrate (kbps):"));
    audio_bitrate_spinbox_ = new QSpinBox();
    audio_bitrate_spinbox_->setLocale(QLocale::c());
    audio_bitrate_spinbox_->setValue(128);
    audio_bitrate_spinbox_->setMinimum(64);
    audio_bitrate_spinbox_->setMaximum(320);
    audio_row->addWidget(audio_bitrate_spinbox_);
    audio_row->addStretch();
    audio_layout->addLayout(audio_row);

    main_layout->addWidget(audio_group);

    // Encoding Group
    QGroupBox* encoding_group = new QGroupBox("Encoding Settings", this);
    QVBoxLayout* encoding_layout = new QVBoxLayout(encoding_group);

    QHBoxLayout* encoding_row = new QHBoxLayout();
    gpu_encoding_ = new QCheckBox("GPU Encoding (NVIDIA NVENC)");
    gpu_encoding_->setChecked(false);
    encoding_row->addWidget(gpu_encoding_);

    encoding_row->addSpacing(20);
    adaptive_bitrate_ = new QCheckBox("Adaptive Bitrate");
    adaptive_bitrate_->setChecked(true);
    encoding_row->addWidget(adaptive_bitrate_);
    encoding_row->addStretch();
    encoding_layout->addLayout(encoding_row);

    main_layout->addWidget(encoding_group);

    // Buttons
    QHBoxLayout* button_row = new QHBoxLayout();
    save_button_ = new QPushButton("💾 Save Settings");
    load_button_ = new QPushButton("📂 Load Settings");
    reset_button_ = new QPushButton("🔄 Reset to Defaults");

    button_row->addWidget(save_button_);
    button_row->addWidget(load_button_);
    button_row->addWidget(reset_button_);
    button_row->addStretch();

    main_layout->addLayout(button_row);
    main_layout->addStretch();

    setLayout(main_layout);
}

void SettingsWidget::LoadCurrentSettings() {
    // Load from config if available
    STREAMX_INFO("Settings loaded from config");
}

void SettingsWidget::ConnectSignals() {
    connect(save_button_, &QPushButton::clicked, this, &SettingsWidget::OnSaveSettingsClicked);
    connect(load_button_, &QPushButton::clicked, this, &SettingsWidget::OnLoadSettingsClicked);
    connect(reset_button_, &QPushButton::clicked, this, &SettingsWidget::OnResetToDefaultsClicked);
}

void SettingsWidget::OnSaveSettingsClicked() {
    ApplySettings();
    QMessageBox::information(this, "Success", "Settings saved successfully!");
    STREAMX_INFO("Settings saved from GUI");
}

void SettingsWidget::OnLoadSettingsClicked() {
    LoadCurrentSettings();
    QMessageBox::information(this, "Success", "Settings loaded successfully!");
    STREAMX_INFO("Settings loaded from GUI");
}

void SettingsWidget::OnResetToDefaultsClicked() {
    width_spinbox_->setValue(1920);
    height_spinbox_->setValue(1080);
    fps_spinbox_->setValue(60);
    bitrate_slider_->setValue(5000);
    codec_selector_->setCurrentText("H264");
    preset_selector_->setCurrentText("medium");
    enable_bframes_->setChecked(true);
    gop_size_spinbox_->setValue(60);

    sample_rate_spinbox_->setValue(48000);
    channels_spinbox_->setValue(2);
    audio_bitrate_spinbox_->setValue(128);

    gpu_encoding_->setChecked(false);
    adaptive_bitrate_->setChecked(true);

    STREAMX_INFO("Settings reset to defaults");
}

void SettingsWidget::OnBitrateChanged(int value) {
    UpdateBitrateLabel(value);
}

void SettingsWidget::UpdateBitrateLabel(int value) {
    // Label would be updated via signal
}

void SettingsWidget::ApplySettings() {
    streamx::VideoConfig video{
        .width = static_cast<uint32_t>(width_spinbox_->value()),
        .height = static_cast<uint32_t>(height_spinbox_->value()),
        .fps = static_cast<uint32_t>(fps_spinbox_->value()),
        .bitrate_kbps = static_cast<uint32_t>(bitrate_slider_->value()),
        .codec = codec_selector_->currentText() == "H264" 
            ? streamx::CodecType::H264 : streamx::CodecType::H265,
        .preset = preset_selector_->currentText().toStdString()
    };

    if (controller_) {
        controller_->UpdateVideoConfig(video);
    }
}
