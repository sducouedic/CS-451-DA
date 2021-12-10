#pragma once

#include <fstream>
#include <vector>
#include <tuple>
#include <climits>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SRC_ID_SIZE 1
#define SEQ_SIZE 4

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

/// (src_id, seq_nr) Identifies a message received from a sender
typedef std::pair<int, int> MessageFrom;

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

    /// Encode a message into a sequence of characters
    static void encode_message(const Message &message, char *buffer, int buffer_size)
    {
        int msg_size = buffer_size - SEQ_SIZE - SRC_ID_SIZE;

        // set src_id
        char src_id = static_cast<char>(message.src_id);
        buffer[0] = src_id;

        // set seq_nr
        int seq_nr = message.seq_nr;
        memcpy(buffer + SRC_ID_SIZE, reinterpret_cast<char *>(&seq_nr), SEQ_SIZE);

        // set message
        strncpy(buffer + SEQ_SIZE + SRC_ID_SIZE, message.msg, msg_size);
        buffer[buffer_size - 1] = '\0';

        if (message.seq_nr != 0 and buffer[SRC_ID_SIZE] == 0 and buffer[SRC_ID_SIZE + 1] == 0 and buffer[SRC_ID_SIZE + 2] == 0 and buffer[SRC_ID_SIZE + 3] == 0)
        {
            std::cerr << message.seq_nr << " encoding error" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    /// Extract a message from a sequence of characters
    static void extract_message(Message &message, const char *buffer, int msg_size)
    {
        // extract src_id
        int src_id = static_cast<int>(buffer[0]);
        if (src_id == CHAR_MIN)
        {
            src_id = CHAR_MAX + 1;
        }
        message.src_id = src_id;

        // extract seq_nr
        memcpy(&(message.seq_nr), buffer + SRC_ID_SIZE, SEQ_SIZE);

        // extract msg
        char *msg = static_cast<char *>(malloc(msg_size));
        strncpy(msg, buffer + SRC_ID_SIZE + SEQ_SIZE, msg_size);
        msg[msg_size - 1] = '\0';
        message.msg = msg;
    }

protected:
    BroadcastUnit *upper_layer;            // upper layer to whom to deliver the message
    int id;                                // id of the current process
    const std::vector<Process> &processes; // list of processes
    volatile bool &stop_flag;              // flag that indicates process failure
};
