#include "network_unit.hpp"

NetworkUnit::NetworkUnit(const std::vector<Process> &processes, int host_id)
{
    setup_sockaddrs(processes);

    this->tcp = new ApproxTCP(sockaddr_from_id(host_id), this);
}

void NetworkUnit::start_networking()
{
    tcp->start();
}

void NetworkUnit::network_send(int dest_id, int seq_nr, const char *msg)
{
    tcp->socket_send(sockaddr_from_id(dest_id), seq_nr, msg);
}

const sockaddr_in *NetworkUnit::sockaddr_from_id(int id)
{
    if (id <= static_cast<int>(sockaddrs.size()))
    {
        return &(sockaddrs[id - 1]);
    }
    return nullptr;
}

int NetworkUnit::id_from_sockaddr(const sockaddr_in *addr)
{
    int len(static_cast<int>(sockaddrs.size()));
    for (int i = 0; i < len; i++)
    {
        sockaddr_in ref = sockaddrs[i];
        if (ref.sin_port == addr->sin_port && ref.sin_addr.s_addr == addr->sin_addr.s_addr)
        {
            return i + 1;
        }
    }
    return -1;
}

const sockaddr_in *NetworkUnit::sockaddr_local(const sockaddr_in &addr)
{
    for (auto &a : sockaddrs)
    {
        if (a.sin_addr.s_addr == addr.sin_addr.s_addr &&
            a.sin_port == addr.sin_port &&
            addr.sin_family == AF_INET)
        {
            return &a;
        }
    }
    return nullptr;
}

void NetworkUnit::setup_sockaddrs(const std::vector<Process> &processes)
{
    sockaddrs.resize(processes.size());

    for (auto &p : processes)
    {
        sockaddr_in sockaddr;
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = p.ip;
        sockaddr.sin_port = p.port;

        sockaddrs[p.id - 1] = sockaddr;
    }
}