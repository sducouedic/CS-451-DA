#pragma once

#include <fstream>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>

/// A simple abstraction of a process
struct Process
{
    int id; // Process id

    in_addr_t ip;
    unsigned short port;
};

/// A simple abstraction of a message
struct Message
{
    int seq_nr; // Message sequence number
    char *msg;  // Proper message
};

/// A Generic class for any layer unit of a broadcast application
class BroadcastUnit
{
public:
    /// Class constructor
    BroadcastUnit(const std::vector<Process> *processes, int id, volatile bool *stop_flag)
        : id(id), processes(*processes), stop_flag(*stop_flag) {}

    /// Class destructor: default
    ~BroadcastUnit() = default;

    /// Broadcast a message (does nothing by default)
    virtual void broadcast(const Message &message){};

    /// Send a messagage to a recipient host
    virtual void send(int dest_id, const Message &message) = 0;

    /// Receive a message from a source host
    virtual void receive(int src_id, const Message &message) = 0;

    /// Write current (frozen) state into output file (does nothing by default)
    virtual void log_state(std::ofstream &file){};

protected:
    /// Deliver a message from a source host to the upper layer
    virtual void deliver(int src_id, const Message &message) = 0;

protected:
    int id;                                // id of the current process
    const std::vector<Process> &processes; // list of processes
    volatile bool &stop_flag;              // flag that indicates process failure
};
