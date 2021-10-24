#pragma once

#include "network_unit.hpp"
#include "approx_tcp.hpp"

/// Define the PerfectLink as seen during the lectures
class PerfectLink : public NetworkUnit
{
public:
    /// Class constructor : setup the list of hosts addresses then open and bind a socket
    PerfectLink(const std::vector<Process> &processes, int id, volatile bool *stop_flag);

    /// TODO Send a message to a recipient host (@see BroadcastUnit)
    void send(int dest_id, const char *msg) override;

    /// TODO Receive a message from a source host (@see BroadcastUnit)
    void receive(int src_id, const char *msg) override;

    /// finish et and log into outputs
    void log_state(std::ofstream &file) override;

    /// get tcp
    ApproxTCP *getTCP() { return tcp; }

protected:
    /// TODO Deliver a message from a source host to the upper layer (@see BroadcastUnit)
    void deliver(int src_id, const char *msg);

private:
    int id; // host id

    ApproxTCP *tcp; // lower level to send message

    std::vector<MessageId> delivered;

    std::vector<int> broadcasted;

    volatile bool &stop_flag; // flag to immediately stop processing msg
};
