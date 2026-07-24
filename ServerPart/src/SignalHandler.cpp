#include "../include/SignalHandler.h"

std::atomic<bool> SignalHandler::shutdown_requested_{false};

void SignalHandler::handleSignal(int s) {
    shutdown_requested_.store(true, std::memory_order_release);
}

void SignalHandler::s_Setup() {
    struct sigaction sigHandler{};

    sigHandler.sa_handler = handleSignal;
    sigemptyset(&sigHandler.sa_mask);
    sigHandler.sa_flags = 0;

    sigaction(SIGINT, &sigHandler, nullptr);
    sigaction(SIGTERM, &sigHandler, nullptr);
}

bool SignalHandler::s_isShutdownRequested() {
    return shutdown_requested_.load(std::memory_order_acquire);
}