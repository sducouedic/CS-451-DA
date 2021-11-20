#pragma once

#include "network_unit.hpp"
#include "approx_tcp.hpp"

/// Define the PerfectLink as seen during the lectures
class PerfectLink : public NetworkUnit
{
public:
    /// Class constructor : setup the list of hosts addresses then open and bind a socket
    PerfectLink(const std::vector<Process> &processes, int id, volatile bool *stop_flag);

    /// (@see BroadcastUnit) Send a message to a recipient host
    void send(int dest_id, int seq_nr, const char *msg) override;

    ///  (@see BroadcastUnit) Receive a message from a source host
    void receive(int src_id, int seq_nr, const char *msg) override;

    /// finish et and log into outputs
    void log_state(std::ofstream &file) override;

    /// get tcp TODO remove and start by hand
    ApproxTCP *getTCP() { return tcp; }

protected:
    /// (@see BroadcastUnit) Deliver a message from a source host to the upper layer
    void deliver(int src_id, int seq_nr, const char *msg) override;

private:
    int id; // host id

    // TODO move to NetworkUnit
    ApproxTCP *tcp; // lower level to send message

    std::vector<MessageId> delivered;

    std::vector<int> broadcasted;

    volatile bool &stop_flag; // flag to immediately stop processing msg
};
