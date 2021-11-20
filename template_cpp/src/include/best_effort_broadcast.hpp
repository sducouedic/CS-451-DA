#pragma once

#include "perfect_link.hpp"

/// An implementation of best effort broadcast
class BestEffortBroadcast : public BroadcastUnit
{
public:
    /// Class constructor
    BestEffortBroadcast(const std::vector<Process> *processes, int id, volatile bool *stop_flag,
                        PerfectLink *perfect_link, BroadcastUnit *upper_layer = nullptr);

    /// (@see BroadcastUnit) Broadcast a message
    void broadcast(const Message &message) override;

    /// (@see BroadcastUnit) Send a message to a recipient host
    void send(int dest_id, const Message &message) override;

    /// (@see BroadcastUnit) Receive a message from a source host
    void receive(int src_id, const Message &message) override;

protected:
    /// (@see BroadcastUnit) Deliver a message from a source host to the upper layer
    void deliver(int src_id, const Message &message) override;

private:
    PerfectLink *perfect_link;
    BroadcastUnit *upper_layer; // upper layer to whom to deliver the message
};
