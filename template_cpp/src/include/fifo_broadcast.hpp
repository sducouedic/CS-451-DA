#pragma once

#include "uniform_rel_broadcast.hpp"
#include "atomic_deque.hpp"

#include <vector>

#define MSG_SIZE_FIFO (MSG_SIZE_PL - SRC_ID_SIZE - SEQ_SIZE)

/// An implementation of fifo causal broadcast
class FIFOBroadcast : public BroadcastUnit
{
public:
    /// Class constructor
    FIFOBroadcast(const std::vector<Process> *processes, int id, volatile bool *stop_flag,
                  PerfectLink *perfect_link, BroadcastUnit *upper_layer = nullptr);

    /// Class destructor
    virtual ~FIFOBroadcast() { delete (uniform_rel_broadcast); }

    /// (@see BroadcastUnit) Broadcast a message
    void broadcast(const Message &message) override;

    /// (@see BroadcastUnit) Send a message to a recipient host
    void send(int dest_id, const Message &message) override;

    /// (@see BroadcastUnit) Receive a message from a source host
    void receive(int src_id, const Message &message) override;

    /// (@see BroadcastUnit) Write current (frozen) state into output file
    void log_state(std::ofstream &file) override;

protected:
    /// (@see BroadcastUnit) Deliver a message from a source host to the upper layer
    void deliver(int src_id, const Message &message) override;

private:
    UniformRelBroadcast *uniform_rel_broadcast;

    int vector_clock[MAX_PROCESSES] = {};       // vector clocks
    std::vector<std::list<Message *>> pendings; // pending messages associated to a source host

    AtomicDeque<Event> events; // list of Events (broadcasts or deliveries)
    // int last_broadcasted_seq_num; // sequence number of last broadcasted message

private:
    // deliver all pending messages from a given process, for which the sequence number
    // are below or equal to the vector clock value associated to that process
    void deliver_pending(int src_id);
};
