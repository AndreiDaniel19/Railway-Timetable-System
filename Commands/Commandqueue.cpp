#include "Commandqueue.h"

using namespace std;

void CommandQueue::push(unique_ptr<Command> cmd) {
    {
        lock_guard<mutex> lock(mtx);
        q.push(move(cmd));
    }
    // Notify the worker that a new command has appeared
    cv.notify_one();
}

unique_ptr<Command> CommandQueue::pop() {
    unique_lock<mutex> lock(mtx);
    
    // Wait until the queue is NOT empty
    // The thread "sleeps" here and does not consume resources
    cv.wait(lock, [&]{ return !q.empty(); });

    // Pop the command
    auto cmd = move(q.front());
    q.pop();
    return cmd;
}