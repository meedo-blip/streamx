# StreamX GUI Implementation Reference

## Overview

This document describes the Qt 6-based GUI implementation for StreamX, including widget architecture, signal/slot mechanisms, and integration with the core streaming engine.

---

## Widget Architecture

### MainWindow (Main Application Container)
**File**: `src/gui/main_window.cpp` / `include/streamx/gui/main_window.h`

**Roles:**
- Application entry point (creates QApplication)
- Manages tab widget with 4 main tabs
- Orchestrates QT signal/slot connections
- Handles window lifecycle and cleanup

**Key Components:**
```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
    std::unique_ptr<streamx::StreamingController> controller_;
    QTabWidget* tab_widget_;
    StreamControlWidget* stream_control_;
    MonitoringWidget* monitoring_;
    PlatformsWidget* platforms_;
    SettingsWidget* settings_;
    
    QLabel* status_label_;
    QLabel* connection_status_;
    QLabel* bitrate_label_;
};
```

**Responsibilities:**
1. Initialize StreamingController with default video/audio config
2. Create and configure all child widgets
3. Create menu bar (File, View, Help menus)
4. Create status bar with real-time indicators
5. Apply dark theme stylesheet
6. Start update timer (refreshes status every 1 second)
7. Handle window close events (graceful shutdown)

**Signals/Slots:**
- `StreamStarted()` → Updates status to "Streaming"
- `StreamStopped()` → Updates status to "Ready"
- `Error()` → Displays error message in status

---

### StreamControlWidget (Streaming Control Tab)
**File**: `src/gui/stream_control_widget.cpp` / `include/streamx/gui/stream_control_widget.h`

**Purpose**: Control capture and streaming operations

**UI Components:**
```
┌─ Capture Settings ─────────────────────┐
│ Capture Type: [Desktop ▼]              │
└────────────────────────────────────────┘

┌─ Stream Control ───────────────────────┐
│ ▶ Start Streaming    ⏹ Stop Streaming │
│ Status: Ready                           │
│ CPU: ████░░░░░░░░░  GPU: ░░░░░░░░░░░  │
└────────────────────────────────────────┘

┌─ Platform Management ──────────────────┐
│ Platforms: [Twitch ▼]  ➕ ❌           │
└────────────────────────────────────────┘
```

**Key Features:**
1. Capture type selector (ComboBox)
2. Start/Stop buttons (QPushButton)
3. Status indicator LED and text
4. CPU/GPU usage progress bars
5. Platform selector and management buttons

**Signal/Slot Flow:**

```
User clicks Start Button
    ↓
OnStartClicked()
    ├→ Get capture type from selector
    ├→ Call controller_->StartCapture()
    ├→ Call controller_->StartStreaming()
    ├→ Update UI state (enable/disable buttons)
    └→ Emit StreamStarted() signal
        ↓
    Main Window receives signal
    Updates status bar
    Updates timer refresh rate
```

**Error Handling:**
- Catches exceptions from controller
- Emits Error() signal with message
- Displays in status bar
- Gracefully recovers to idle state

---

### MonitoringWidget (Real-time Metrics Tab)
**File**: `src/gui/monitoring_widget.cpp` / `include/streamx/gui/monitoring_widget.h`

**Purpose**: Display real-time streaming metrics

**UI Layout:**
```
┌─ Overall Stream Health ────────────────┐
│ Status: 🟢 Connected                   │
│ Overall Bitrate: ████████░░░░░░░░░░░░ │
│ Avg Latency:     ░░░░░░░░░░░░░░░░░░░░ │
│ Frames: 12450   Latency: 45ms          │
└────────────────────────────────────────┘

┌─ Per-Platform Metrics ─────────────────┐
│ Platform │ Status │ Bitrate│ Latency   │
├──────────┼────────┼─────────┼──────────┤
│ Twitch   │ 🟢     │ 4850   │ 32ms     │
│ YouTube  │ 🟢     │ 4920   │ 48ms     │
│ StreamLab│ 🔴     │ 0      │ ∞        │
└────────────────────────────────────────┘
```

**Components:**
- Overall health label with emoji status indicator
- Bitrate progress bar (visualization of 10 Mbps max)
- Latency progress bar (visualization of 5 sec max)
- Statistics labels (frames, uptime)
- QTableWidget for per-platform metrics

**Update Mechanism:**
```cpp
QTimer update_timer;  // 500ms interval

QTimer timeout:
    UpdateMetrics()
        ├→ Get overall health from controller
        ├→ Update progress bars
        ├→ Update status label color (green/red)
        └→ Update stats labels
    
    RefreshPlatformTable()
        ├→ Get platform health map from controller
        ├→ Iterate through platforms
        ├→ Update table rows with metrics
        └→ Refresh color indicators
```

**Data Flow:**
1. Monitor timer fires every 500ms
2. Query StreamingController for metrics
3. Update UI progress bars and labels
4. Rebuild platform table with latest data

---

### PlatformsWidget (Platform Management Tab)
**File**: `src/gui/platforms_widget.cpp` / `include/streamx/gui/platforms_widget.h`

**Purpose**: Manage streaming platform configurations

**UI Components:**
```
┌─ Platforms Management ─────────────────┐
│ Platform │ Type   │ Key  │ Status      │
├──────────┼────────┼──────┼─────────────┤
│ Twitch   │ Twitch │ **** │ 🟢          │
│ YouTube  │ YouTube│ **** │ 🟢          │
├──────────┴────────┴──────┴─────────────┤
│ ➕ Add  ✏️ Edit  ❌ Remove              │
└────────────────────────────────────────┘
```

**Platform Types:**
- Twitch (Direct RTMP)
- YouTube (RTMP/HLS)
- StreamLabs (Custom integration)
- Custom RTMP (Generic RTMP servers)

**Operations:**

**Add Platform:**
```
User clicks Add Button
    ↓
ShowAddPlatformDialog()
    ├→ Show platform type selector
    ├→ Get user selection (ComboBox)
    ├→ Request stream key (SecureLineEdit)
    └→ Call controller_->AddPlatform()
        ├→ Validate connection
        ├→ Store credentials
        └→ Refresh UI table
```

**Edit Platform:**
```
User selects row + clicks Edit
    ↓
OnEditClicked()
    ├→ Get currently selected platform
    ├→ Request new stream key
    └→ Update StreamingController
        └→ Refresh UI table
```

**Remove Platform:**
```
User selects row + clicks Remove
    ↓
OnRemovePlatformClicked()
    ├→ Confirm removal
    ├→ Call controller_->RemovePlatform()
    └→ Refresh UI table
```

**Table Management:**
- QTableWidget with 4 columns: Name, Type, Key, Status
- Stream keys masked as "****" for security
- Selectable rows for edit/delete operations
- Status indicators (🟢/🔴) for connection state

---

### SettingsWidget (Settings Tab)
**File**: `src/gui/settings_widget.cpp` / `include/streamx/gui/settings_widget.h`

**Purpose**: Configure video, audio, and encoding parameters

**Layout:**
```
╔═ Video Settings ════════════════════════╗
║ Resolution: [1920] x [1080]             ║
║ FPS: [60]          Bitrate: [5000] kbps║
║ Codec: [H264 ▼]    Preset: [medium ▼] ║
║ ☑ Enable B-Frames  GOP Size: [60]      ║
╚═════════════════════════════════════════╝

╔═ Audio Settings ════════════════════════╗
║ Sample Rate: [48000] Hz                 ║
║ Channels: [2]      Bitrate: [128] kbps ║
╚═════════════════════════════════════════╝

╔═ Encoding Settings ═════════════════════╗
║ ☑ GPU Encoding (NVIDIA NVENC)           ║
║ ☑ Adaptive Bitrate                      ║
╚═════════════════════════════════════════╝

[💾 Save] [📂 Load] [🔄 Reset]
```

**Input Controls:**
- `QSpinBox`: Width, Height, FPS, Sample Rate, Channels, Audio Bitrate, GOP Size
- `QSlider`: Video Bitrate (visual + numeric)
- `QComboBox`: Codec (H264/H265), Encoding Preset
- `QCheckBox`: B-Frames, GPU Encoding, Adaptive Bitrate

**Preset Values:**

| Parameter | Min | Default | Max |
|-----------|-----|---------|-----|
| Resolution | 640×480 | 1920×1080 | 4096×2160 |
| FPS | 24 | 60 | 120 |
| Video Bitrate | 1000 | 5000 | 10000 (kbps) |
| Audio Sample Rate | 16000 | 48000 | 192000 (Hz) |
| Audio Channels | 1 | 2 | 8 |
| Audio Bitrate | 64 | 128 | 320 (kbps) |
| GOP Size | 1 | 60 | 300 (frames) |

**Actions:**

`Save Settings:`
```cpp
OnSaveSettingsClicked()
    ├→ ApplySettings()
    │   ├→ Create VideoConfig from UI values
    │   └→ Call controller_->UpdateVideoConfig()
    ├→ Save to config.json
    └→ Show success message
```

`Load Settings:`
```cpp
OnLoadSettingsClicked()
    ├→ LoadCurrentSettings()
    │   ├→ Read config.json
    │   └→ Update UI controls
    └→ Show success message
```

`Reset to Defaults:`
```cpp
OnResetToDefaultsClicked()
    ├→ Set all spinboxes to defaults
    ├→ Set all sliders to defaults
    ├→ Reset checkboxes
    └→ Show confirmation
```

---

## Signal/Slot Architecture

### Connection Hierarchy

```
MainWindow
    ├─ StreamControlWidget
    │   ├─ StreamStarted() → MainWindow::OnStreamStarted()
    │   ├─ StreamStopped() → MainWindow::OnStreamStopped()
    │   └─ Error() → MainWindow::OnError()
    │
    ├─ MonitoringWidget
    │   ├─ update_timer timeout → UpdateMetrics()
    │   └─ update_timer timeout → RefreshPlatformTable()
    │
    ├─ PlatformsWidget
    │   ├─ add_button clicked → OnAddPlatformClicked()
    │   ├─ remove_button clicked → OnRemovePlatformClicked()
    │   └─ edit_button clicked → OnEditClicked()
    │
    └─ SettingsWidget
        ├─ save_button clicked → OnSaveSettingsClicked()
        ├─ load_button clicked → OnLoadSettingsClicked()
        ├─ reset_button clicked → OnResetToDefaultsClicked()
        └─ bitrate_slider value changed → UpdateBitrateLabel()
```

---

## Qt Meta-Object System

All widgets use Qt's Meta-Object Compiler (MOC):

**Header Requirements:**
```cpp
class MyWidget : public QWidget {
    Q_OBJECT  // Required for MOC to process class
    
public slots:  // Auto-connected to signals
    void OnButtonClicked();
    
signals:  // Emitted to trigger slot execution
    void DataChanged(const QString& data);
};
```

**CMake Support:**
```cmake
set(CMAKE_AUTOMOC ON)    # Automatically run MOC
set(CMAKE_AUTORCC ON)    # Process .qrc files
set(CMAKE_AUTOUIC ON)    # Process .ui files
```

---

## Threading Model

**Main Thread:** All Qt UI operations (required)
**Worker Threads:** Managed by StreamingController
  - Capture thread
  - Encoding thread
  - Transmission thread (per-platform)

**Thread-Safe Queue for Data:**
```cpp
// Frame passing: Lock-free ring buffer
FrameBuffer<VideoFrame> frame_queue;

// Status updates: Copy metrics to atomic variables
std::atomic<StreamHealth> health_metrics;
std::atomic<bool> is_streaming;
```

**UI Update Safe Pattern:**
```cpp
// Called from worker thread - NOT safe for Qt
void StreamingController::UpdateMetrics(const StreamHealth& h) {
    health_metrics = h;  // Atomic copy
}

// Called from main thread - SAFE for Qt
void MonitoringWidget::UpdateMetrics() {
    auto health = controller_->GetOverallHealth();
    ui_label->setText(...);  // Now we're in main thread
}
```

---

## Resource Management

### Resources File (resources.qrc)
```xml
<RCC>
    <qresource>
        <file>icons/streamx.png</file>
        <file>icons/start.png</file>
        <file>icons/stop.png</file>
        <file>icons/settings.png</file>
        <file>styles/dark.qss</file>
    </qresource>
</RCC>
```

**Usage in Code:**
```cpp
setWindowIcon(QIcon(":/icons/streamx.png"));  // From resources
```

### Memory Management
- Widgets created as heap objects (new)
- MainWindow owns all child widgets
- Qt's parent-child relationship handles cleanup
- StreamingController owned by unique_ptr

---

## Stylesheet System

**Dark Theme Applied in MainWindow:**
```cpp
QString stylesheet = R"(
    QMainWindow { background-color: #2b2b2b; color: #ffffff; }
    QTabBar::tab:selected { background-color: #0d47a1; }
    QPushButton:hover { background-color: #1565c0; }
    QProgressBar::chunk { background-color: #0d47a1; }
)";
qApp->setStyleSheet(stylesheet);
```

**Color Palette:**
- Background: #2b2b2b (Dark Gray)
- Accent: #0d47a1 (Deep Blue)
- Success: #4caf50 (Green)
- Error: #f44336 (Red)
- Text: #ffffff (White)

---

## Error Handling

**Widget-Level Error Handling:**
```cpp
try {
    if (!controller_->StartStreaming()) {
        emit Error("Failed to start streaming");
    }
} catch (const std::exception& e) {
    emit Error(QString::fromStdString(e.what()));
}
```

**MainWindow-Level Error Display:**
```cpp
void MainWindow::OnError(const QString& error) {
    STREAMX_ERROR("GUI Error: " + error.toStdString());
    status_label_->setText("Error: " + error);
    status_label_->setStyleSheet("color: #f44336;");  // Red
}
```

---

## Example: Complete User Workflow

**User starts streaming to Twitch:**

```
1. User launches: ./streamx_gui
   ↓
2. MainWindow creates StreamingController
   ↓
3. User goes to Platforms tab
   ↓
4. Clicks "Add Platform" button
   │   ├→ Shows platform selector dialog
   │   ├→ User selects "Twitch"
   │   └→ Requests stream key (masked input)
   │
5. User goes to Settings tab
   │   ├→ Adjusts resolution to 1920×1080
   │   ├→ Sets FPS to 60
   │   ├→ Selects H.264 codec
   │   └→ Clicks "Save Settings"
   │
6. User goes to Stream Control tab
   │   ├→ Selects "Desktop" capture
   │   ├→ Clicks "▶ Start Streaming"
   │   │   ├→ Starts desktop capture
   │   │   ├→ Initialize encoder (H.264 @ 1920×1080 60fps)
   │   │   ├→ Connects to Twitch RTMP server
   │   │   └→ Begins streaming
   │   │
   │   ├→ Status updates to "Streaming 🟢"
   │   └→ Start button disabled, Stop button enabled
   │
7. User checks Monitoring tab
   │   ├→ Updates every 500ms
   │   ├→ Shows "🟢 Connected"
   │   ├→ Displays real-time bitrate: 4850 kbps
   │   ├→ Shows latency: 32ms
   │   └→ Platform table shows Twitch connected
   │
8. User clicks "⏹ Stop Streaming"
   │   ├→ Stops capture thread
   │   ├→ Flushes encoder buffer
   │   ├→ Closes RTMP connection
   │   └→ Resets UI state
   │
9. Status updates to "Ready" 🟢
   ├→ Start button enabled
   └→ Stop button disabled
```

---

## Code Examples

### Adding a New Platform in Settings

```cpp
// In SettingsWidget.h - Add dropdown for new platform
QComboBox* video_effect_selector_;

// In SettingsWidget::CreateUI()
settings_layout->addWidget(new QLabel("Video Effect:"));
video_effect_selector_ = new QComboBox();
video_effect_selector_->addItems({"None", "Blur", "Grayscale"});
settings_layout->addWidget(video_effect_selector_);

// In SettingsWidget::ApplySettings()
if (video_effect_selector_->currentText() != "None") {
    controller_->SetVideoEffect(
        video_effect_selector_->currentText().toStdString()
    );
}
```

### Extending Monitoring

```cpp
// In MonitoringWidget - Add CPU/GPU gauge
QLabel* gpu_temp_label_;

// In CreateUI()
gpu_temp_label_ = new QLabel("GPU Temp: 0°C");
health_layout->addWidget(gpu_temp_label_);

// In UpdateMetrics()
auto health = controller_->GetOverallHealth();
gpu_temp_label_->setText(
    QString("GPU Temp: %1°C").arg(health.gpu_temperature)
);
```

---

## Best Practices Applied

✓ **RAII**: Resources automatically cleaned up
✓ **Signal/Slot**: Loose coupling between components
✓ **Thread Safety**: Atomic variables for shared state
✓ **Error Handling**: Try-catch with user feedback
✓ **Resource Cleanup**: Parent-child Qt relationships
✓ **Responsive UI**: Update timer prevents blocking
✓ **Code Reuse**: Shared StreamingController between CLI and GUI
✓ **Documentation**: Comprehensive inline comments
✓ **Scalability**: Easy to add new widgets and tabs

---

## Performance Considerations

- Update timer: 500ms (non-blocking)
- Status bar update: 1000ms
- Table refresh rate: 500ms
- All blocking operations in worker threads
- No Qt operations on non-main threads
- Zero-copy frame passing via ring buffers

---

## Testing the GUI

```bash
# Build with debug symbols for debugging
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_GUI=ON
cmake --build .

# Run with Qt debugging
export QT_DEBUG_PLUGINS=1
./streamx_gui

# Run with GDB
gdb ./streamx_gui
```

---

## Conclusion

The StreamX GUI is a well-architected, extensible Qt6 application that:
- Maintains clean separation between UI and business logic
- Uses Qt's signal/slot mechanism for safe inter-widget communication
- Efficiently updates real-time metrics without blocking
- Provides professional UX with dark theme
- Integrates seamlessly with the core streaming engine
