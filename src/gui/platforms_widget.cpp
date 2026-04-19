#include <QCoreApplication>
#include <sys/types.h>
#include <pwd.h>

#include "streamx/gui/platforms_widget.h"
#include "streamx/utils/logger.h"
#include "streamx/utils/config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <iostream>

static std::string GetConfigPath() {
    struct passwd* pw = getpwuid(getuid());
    std::string config_dir = std::string(pw ? pw->pw_dir : "/home") + "/.config/streamx";
    return config_dir + "/config.json";
}

PlatformsWidget::PlatformsWidget(streamx::StreamingController* controller, QWidget* parent)
    : QWidget(parent), controller_(controller) {
    CreateUI();
    SetupTable();
    ConnectSignals();
}

void PlatformsWidget::CreateUI() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);

    // Platforms table
    platforms_table_ = new QTableWidget(this);
    main_layout->addWidget(platforms_table_);

    // Button row
    QHBoxLayout* button_layout = new QHBoxLayout();
    
    add_button_ = new QPushButton("➕ Add Platform");
    remove_button_ = new QPushButton("❌ Remove");
    edit_button_ = new QPushButton("✏️ Edit");

    button_layout->addWidget(add_button_);
    button_layout->addWidget(edit_button_);
    button_layout->addWidget(remove_button_);
    button_layout->addStretch();

    main_layout->addLayout(button_layout);

    setLayout(main_layout);
}

void PlatformsWidget::SetupTable() {
    platforms_table_->setColumnCount(4);
    platforms_table_->setHorizontalHeaderLabels({
        "Platform Name", "Type", "Stream Key", "Status"
    });

    platforms_table_->horizontalHeader()->setStretchLastSection(true);
    platforms_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    platforms_table_->setSelectionMode(QAbstractItemView::SingleSelection);
}

void PlatformsWidget::ConnectSignals() {
    connect(add_button_, &QPushButton::clicked, this, &PlatformsWidget::OnAddPlatformClicked);
    connect(remove_button_, &QPushButton::clicked, this, &PlatformsWidget::OnRemovePlatformClicked);
    connect(edit_button_, &QPushButton::clicked, this, &PlatformsWidget::OnEditClicked);
}

void PlatformsWidget::ShowAddPlatformDialog() {
    QStringList platforms = {"Twitch", "YouTube", "StreamLabs", "Custom RTMP"};
    bool ok;
    QString platform_name = QInputDialog::getItem(this, "Add Platform", 
                                                 "Select platform type:",
                                                 platforms, 0, false, &ok);
    if (!ok) return;

    QString stream_key = QInputDialog::getText(this, "Stream Key",
                                              "Enter your stream key:",
                                              QLineEdit::Password, "", &ok);
    if (!ok) return;

    std::string platform_lower = platform_name.toLower().toStdString();
    std::string server_url;
    if (platform_name == "Custom RTMP") {
        QString server = QInputDialog::getText(this, "RTMP Server URL",
                                              "Enter the RTMP server URL:",
                                              QLineEdit::Normal, "", &ok);
        if (!ok || server.isEmpty()) return;
        server_url = server.toStdString();
    }

    if (controller_->AddPlatform(platform_lower, platform_lower,
                                 stream_key.toStdString(), server_url)) {
        // Save platform config
        auto& config = streamx::Config::Instance();
        config["platforms"][platform_lower]["stream_key"] = stream_key.toStdString();
        if (!server_url.empty()) {
            config["platforms"][platform_lower]["server_url"] = server_url;
        }
        config.SaveToFile(GetConfigPath());

        RefreshPlatformsList();
        STREAMX_INFO("Platform added: " + platform_name.toStdString());
    } else {
        QMessageBox::warning(this, "Error", "Failed to add platform");
    }
}

void PlatformsWidget::OnAddPlatformClicked() {
    ShowAddPlatformDialog();
}

void PlatformsWidget::OnRemovePlatformClicked() {
    int current_row = platforms_table_->currentRow();
    if (current_row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a platform first");
        return;
    }

    QString platform_name = platforms_table_->item(current_row, 0)->text();
    if (controller_->RemovePlatform(platform_name.toStdString())) {
        // Remove from config
        auto& config = streamx::Config::Instance();
        config["platforms"].erase(platform_name.toStdString());
        config.SaveToFile(GetConfigPath());

        RefreshPlatformsList();
        STREAMX_INFO("Platform removed: " + platform_name.toStdString());
    }
}

void PlatformsWidget::OnEditClicked() {
    int current_row = platforms_table_->currentRow();
    if (current_row < 0) {
        QMessageBox::warning(this, "No Selection", "Please select a platform first");
        return;
    }

    QString platform_name = platforms_table_->item(current_row, 0)->text();
    QString new_key = QInputDialog::getText(this, "Edit Stream Key",
                                           "Enter new stream key for " + platform_name + ":",
                                           QLineEdit::Password, "", nullptr);
    if (!new_key.isEmpty()) {
        // Update config
        auto& config = streamx::Config::Instance();
        config["platforms"][platform_name.toStdString()]["stream_key"] = new_key.toStdString();
        config.SaveToFile(GetConfigPath());

        STREAMX_INFO("Stream key updated for: " + platform_name.toStdString());
        RefreshPlatformsList();
    }
}

void PlatformsWidget::RefreshPlatformsList() {
    auto all_platforms = controller_->GetAllPlatforms();

    platforms_table_->setRowCount(all_platforms.size());

    int row = 0;
    for (const auto& platform : all_platforms) {
        auto health_map = controller_->GetPlatformHealth();
        const bool connected = health_map.count(platform) > 0 && health_map.at(platform).connected;

        auto name_item = new QTableWidgetItem(QString::fromStdString(platform));
        auto type_item = new QTableWidgetItem(QString::fromStdString(platform));
        auto key_item = new QTableWidgetItem("****");  // Don't show actual key
        auto status_item = new QTableWidgetItem(connected ? "🟢 Connected" : "⚪ Configured");

        platforms_table_->setItem(row, 0, name_item);
        platforms_table_->setItem(row, 1, type_item);
        platforms_table_->setItem(row, 2, key_item);
        platforms_table_->setItem(row, 3, status_item);

        row++;
    }
}
