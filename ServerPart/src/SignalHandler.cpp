#include "../include/SignalHandler.h"

std::atomic<bool> SignalHandler::shutdown_requested{false};

void SignalHandler::handleSignal(int s) {
    shutdown_requested.store(true, std::memory_order_release);
}

void SignalHandler::Setup() {
    struct sigaction sigHandler;

    sigHandler.sa_handler = handleSignal;
    sigemptyset(&sigHandler.sa_mask);
    sigHandler.sa_flags = 0;

    sigaction(SIGINT, &sigHandler, NULL);
    sigaction(SIGTERM, &sigHandler, NULL);
}

bool SignalHandler::isShutdownRequested() {
    return shutdown_requested.load(std::memory_order_acquire);
}