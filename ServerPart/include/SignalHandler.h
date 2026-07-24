#pragma once

#include <csignal>
#include <atomic>

class SignalHandler {
public:
    static void Setup();
    static bool isShutdownRequested();

private:
    static std::atomic<bool> shutdown_requested;

    static void handleSignal(int s);
};