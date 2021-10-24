#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>

#include "broadcast_unit.hpp"

class NetworkUnit : public BroadcastUnit
{
public:
    /// Class Constructor: setup sockaddrs using list of process
    NetworkUnit(const std::vector<Process> &processes);

    /// get the sockaddr of a process id
    sockaddr_in *sockaddr_from_id(int id);

    /// get the id corresponding of a sockaddr
    /// returns -1 if no corresponding process
    int id_from_sockaddr(sockaddr_in *addr);

    /// get a pointer to a locally stored sockaddr, if one is the
    /// same as the given parameter
    sockaddr_in *sockaddr_local(sockaddr_in &addr);

private:
    std::vector<sockaddr_in> sockaddrs; //sockaddrs corresponding to each process

private:
    // initialise sockaddrs
    void setup_sockaddrs(const std::vector<Process> &processes);
};