#pragma once

#include "uniform_rel_broadcast.hpp"
#include "atomic_deque.hpp"

#include <atomic>
#include <vector>
#include <array>

#define MSG_SIZE_LCB (MSG_SIZE_PL - VECTOR_CLOCK_SIZE)

/// Local Causal broadcast add the vector clock to the message
struct LCBMessage : Message
{
    int vector_clock[MAX_PROCESSES] = {}; // vector clocks
};

/// An implementation of localized causal broadcast
class LocalCausalBroadcast : public BroadcastUnit
{
public:
    /// Class constructor
    LocalCausalBroadcast(const std::vector<Process> *processes, int id,
                         const std::vector<int> *affecting_processes, volatile bool *stop_flag,
                         PerfectLink *perfect_link, BroadcastUnit *upper_layer = nullptr);

    /// Class destructor
    virtual ~LocalCausalBroadcast()
    {
        delete (uniform_rel_broadcast);
    }

    /// (@see BroadcastUnit) Broadcast a message
    void broadcast(const Message &message) override;

    /// (@see BroadcastUnit) Send a message to a recipient host
    void send(int dest_id, const Message &message) override;

    /// (@see BroadcastUnit) Receive a message from a source host
    void receive(int src_id, const Message &message) override;

    /// (@see BroadcastUnit) Write current (frozen state) into output file
    void log_state(std::ofstream &file) override;

protected:
    /// (@see BroadcastUnit) Deliver a message from a source host to the upper layer
    void deliver(int src_id, const Message &message) override;

private:
    UniformRelBroadcast *uniform_rel_broadcast;

    int vc_broadcasted;
    std::array<std::atomic<int>, MAX_PROCESSES> vector_clock; // vector clocks
    const std::vector<int> &affecting_processes;              // other processes current process is affected by

    std::vector<std::list<LCBMessage *>> pendings; // pending messages associated to a source host

    AtomicDeque<Event> events; // list of Events (broadcasts or deliveries)

private:
    // deliver all pending messages for with all values in the vector clock has been reached
    void deliver_pending();

    // add current vector clock state to the given message
    void append_vector_clock(Message &message, const char *msg);

    // fill vector clock of the lcb_message given content of a buffer
    void set_message_vector_clock(LCBMessage &lcb_message, const char *buffer);
};
