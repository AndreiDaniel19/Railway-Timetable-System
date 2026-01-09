#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "Command.h"

class CommandQueue {
private:
    std::queue<std::unique_ptr<Command>> q;
    std::mutex mtx;
    std::condition_variable cv;

public:
    // Adds a command to the queue (used by Client Threads)
    void push(std::unique_ptr<Command> cmd);

    // Extracts a command (used by Worker Thread)
    // This function is BLOCKING: it waits until there is something in the queue
    std::unique_ptr<Command> pop();
};