#pragma once

#include "best_effort_broadcast.hpp"

#include <map>
#include <set>
#include <vector>

/// An implementation of uniform reliable broadcast
class UniformRelBroadcast : public BroadcastUnit
{
public:
    /// Class constructor
    UniformRelBroadcast(const std::vector<Process> *processes, int id, volatile bool *stop_flag,
                        PerfectLink *perfect_link, BroadcastUnit *upper_layer = nullptr);

    virtual ~UniformRelBroadcast() { delete (best_eff_broadcast); }

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
    BestEffortBroadcast *best_eff_broadcast;

    // associate a message to the list of processes that acknowledged it
    std::map<MessageFrom, std::vector<int>> message_acks;
    int MIN_ACKS;

    std::vector<MessageFrom> delivered; // list of delivered messages (src_id, seq_nr)
    int last_broadcasted_seq_num;       // sequence number of last broadcasted message

private:
    // add ack from source to message
    // return true if the message can be delivered
    bool insert_ack(const MessageFrom &message, int src_id);

    // determines if a message can be delivered
    bool can_deliver(const MessageFrom &message);
};
