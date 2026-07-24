#pragma once

#include <csignal>
#include <atomic>

class SignalHandler {
public:
    static void s_Setup();
    static bool s_isShutdownRequested();

private:
    static std::atomic<bool> shutdown_requested_;

private:
    static void handleSignal(int s);
};