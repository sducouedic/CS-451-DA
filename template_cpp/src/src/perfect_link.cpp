#include "perfect_link.hpp"
#include <limits.h>
//TEMP
#include <iostream>

PerfectLink::PerfectLink(const std::vector<Process> *processes, int id, volatile bool *stop_flag)
    : NetworkUnit(processes, id, stop_flag) {}

void PerfectLink::send(int dest_id, const Message &message)
{
    std::cout << "PL --> send " << message.seq_nr << " to " << dest_id << std::endl;
    network_send(dest_id, message);
    broadcasted.push_back(message.seq_nr);
}

void PerfectLink::receive(int src_id, const Message &message)
{
    for (auto &d : delivered)
    {
        if (d.first == src_id && d.second == message.seq_nr)
        {
            return;
        }
    }

    // deliver message
    deliver(src_id, message);
}

void PerfectLink::log_state(std::ofstream &file)
{
    for (size_t i = 0; i < broadcasted.size(); ++i)
    {
        file << "b " << broadcasted[i] << "\n";
    }

    for (size_t i = 0; i < delivered.size(); ++i)
    {
        file << "d " << delivered[i].first << " " << delivered[i].second << "\n";
    }
}

void PerfectLink::deliver(int src_id, const Message &message)
{
    // add the message to the list of delivered messages
    // TODO remove stop_flag
    if (!stop_flag)
    {
        delivered.push_back(std::pair<int, int>(src_id, message.seq_nr));

        // TEMP
        std::cout << "PL --> delivers " << message.seq_nr << " from " << src_id << std::endl;
    }
}
