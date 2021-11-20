#pragma once

#include <fstream>

/// A simple abstraction of a process
struct Process
{
    int id; // Process id

    in_addr_t ip;
    unsigned short port;
};

/// A Generic class for any layer unit of a broadcast application
class BroadcastUnit
{
public:
    /// Class constructor: default
    BroadcastUnit() = default;

    /// Class destructor: default
    ~BroadcastUnit() = default;

    /// Send a messagage to a recipient host
    virtual void send(int dest_id, int seq_nr, const char *msg) = 0;

    /// Receive a message from a source host
    virtual void receive(int src_id, int seq_nr, const char *msg) = 0;

    /// Crash of the process
    virtual void log_state(std::ofstream &file) = 0;

protected:
    /// Deliver a message from a source host to the upper layer
    virtual void deliver(int src_id, int seq_nr, const char *msg) = 0;
};
