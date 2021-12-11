#pragma once

#include <fstream>
#include <vector>
#include <tuple>
#include <climits>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_PROCESSES 128

#define SRC_ID_SIZE 1
#define SEQ_SIZE 4
#define VECTOR_CLOCK_SIZE (4 * MAX_PROCESSES)

/// A simple abstraction of a process
struct Process
{
    int id; // Process id
    in_addr_t ip;
    unsigned short port;
};

/// An simple abstraction of a message
/// Important: this abstraction is valid at every level, BUT msg recursively
/// contain (seq_nr, src_id) of the upper level
struct Message
{
    int src_id = 0;      // Sender of the message
    int seq_nr = 0;      // Message sequence number
    char *msg = nullptr; // Proper message
};

/// Represent an event (either broadcast or deliver event)
struct Event
{
    char type = 'a'; // 'b' for broadcast, 'd' for deliver
    int src_id = 0;
    int seq_nr = 0;
};

/// (src_id, seq_nr) Identifies a message received from a sender
typedef std::pair<int, int>
    MessageFrom;

/// A Generic class for any layer unit of a broadcast application
class BroadcastUnit
{
public:
    /// Class constructor
    BroadcastUnit(const std::vector<Process> *processes, int id,
                  volatile bool *stop_flag, BroadcastUnit *upper_layer = nullptr)
        : upper_layer(upper_layer), id(id), processes(*processes), stop_flag(*stop_flag) {}

    /// Class destructor: default
    virtual ~BroadcastUnit() = default;

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
    BroadcastUnit *upper_layer;            // upper layer to whom to deliver the message
    int id;                                // id of the current process
    const std::vector<Process> &processes; // list of processes
    volatile bool &stop_flag;              // flag that indicates process failure
};
