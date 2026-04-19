#include <QApplication>
#include "streamx/gui/main_window.h"
#include "streamx/utils/logger.h"
#include <iostream>

int main(int argc, char** argv) {
    std::cerr << "[GUI] Qt Application initialization starting..." << std::endl;
    QApplication app(argc, argv);
    std::cerr << "[GUI] Qt Application created" << std::endl;

    // Initialize logger
    std::cerr << "[GUI] Initializing logger..." << std::endl;
    streamx::Logger::Instance().Initialize("streamx_gui.log");
    STREAMX_INFO("StreamX GUI Application started");
    std::cerr << "[GUI] Logger initialized" << std::endl;

    // Create and show main window
    std::cerr << "[GUI] Creating MainWindow..." << std::endl;
    MainWindow window;
    std::cerr << "[GUI] MainWindow created" << std::endl;
    window.show();
    std::cerr << "[GUI] MainWindow shown" << std::endl;

    STREAMX_INFO("Main window shown");

    return app.exec();
}
