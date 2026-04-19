#include <QApplication>
#include "streamx/gui/main_window.h"
#include "streamx/utils/logger.h"
#include "streamx/utils/config.h"
#include <iostream>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>

static std::string GetConfigPath() {
    struct passwd* pw = getpwuid(getuid());
    std::string config_dir = std::string(pw ? pw->pw_dir : "/home") + "/.config/streamx";
    mkdir(config_dir.c_str(), 0755);
    return config_dir + "/config.json";
}

int main(int argc, char** argv) {
    std::cerr << "[GUI] Qt Application initialization starting..." << std::endl;
    QApplication app(argc, argv);
    std::cerr << "[GUI] Qt Application created" << std::endl;

    // Initialize logger
    std::cerr << "[GUI] Initializing logger..." << std::endl;
    streamx::Logger::Instance().Initialize("streamx_gui.log");
    STREAMX_INFO("StreamX GUI Application started");
    std::cerr << "[GUI] Logger initialized" << std::endl;

    // Load config from ~/.config/streamx/config.json
    std::string config_path = GetConfigPath();
    streamx::Config::Instance().LoadFromFile(config_path);
    STREAMX_INFO("Settings loaded from config");

    // Create and show main window
    std::cerr << "[GUI] Creating MainWindow..." << std::endl;
    MainWindow window;
    std::cerr << "[GUI] MainWindow created" << std::endl;
    window.show();
    std::cerr << "[GUI] MainWindow shown" << std::endl;

    STREAMX_INFO("Main window shown");

    return app.exec();
}
