#pragma once

#include "network_unit.hpp"

#include <tuple>

/// An implementation of a perfect link
class PerfectLink : public NetworkUnit
{
public:
    /// Class constructor
    PerfectLink(const std::vector<Process> *processes, int id, volatile bool *stop_flag);

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
    std::vector<std::pair<int, int>> delivered; // list of delivered messages (src_id, seq_nr)
    std::vector<int> broadcasted;               // list of broadcasted messages
};
