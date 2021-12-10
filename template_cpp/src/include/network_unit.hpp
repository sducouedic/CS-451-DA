#pragma once

#include <iostream>

#include "broadcast_unit.hpp"
#include "approx_tcp.hpp"

/// The lowest-level layer of the broadcast application; needs to interact with the network
class NetworkUnit : public BroadcastUnit
{
public:
    /// Class Constructor: setup sockaddrs using list of process
    NetworkUnit(const std::vector<Process> *processes, int id,
                volatile bool *stop_flag, BroadcastUnit *upper_layer = nullptr);

    /// Class destructor
    virtual ~NetworkUnit() { delete (tcp); };

    /// Start TCP networking
    void start_networking();

    /// Send message through the network
    void network_send(int dest_id, const Message &network_msg);

    /// get the sockaddr of a process id
    const sockaddr_in *sockaddr_from_id(int id);

    /// get the id corresponding of a sockaddr
    /// returns -1 if no corresponding process
    int id_from_sockaddr(const sockaddr_in *addr);

    /// get a pointer to a locally stored sockaddr, if one is the same as the given parameter
    const sockaddr_in *sockaddr_local(const sockaddr_in &addr);

private:
    std::vector<sockaddr_in> sockaddrs; // sockaddrs corresponding to each process
    ApproxTCP *tcp;                     // lower level to send message

private:
    // initialise sockaddrs
    void setup_sockaddrs(const std::vector<Process> &processes);
};
