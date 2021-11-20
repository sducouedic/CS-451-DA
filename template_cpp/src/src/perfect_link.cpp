#include "perfect_link.hpp"
#include <limits.h>
//TEMP
#include <iostream>

PerfectLink::PerfectLink(const std::vector<Process> &processes, int id, volatile bool *stop_flag)
    : NetworkUnit(processes), id(id), nb_broadcasted(0), stop_flag(*stop_flag)
{
    this->tcp = new ApproxTCP(sockaddr_from_id(id), this);
}

void PerfectLink::send(int dest_id, const char *msg)
{
    // std::cout << "PL send" << std::endl;
    ++nb_broadcasted;
    tcp->socket_send(sockaddr_from_id(dest_id), nb_broadcasted, msg);
    broadcasted.push_back(nb_broadcasted);
}

void PerfectLink::receive_from_network(int src_id, int seq_nr, const char *msg)
{
    //TEMP
    // std::cout << "PL --> src: " << src_id
    //           << ", msg: " << msg << std::endl;

    for (auto &d : delivered)
    {
        if (d.src_id == src_id && d.seq_nr == seq_nr)
        {
            return;
        }
    }

    char *buffer = static_cast<char *>(malloc(MAXLINE));
    strncpy(buffer, msg, MSG_SIZE);
    buffer[MAXLINE - 1] = '\0';

    MessageId *msg_id = new MessageId;
    msg_id->src_id = src_id;
    msg_id->seq_nr = seq_nr;

    deliver(src_id, buffer);
    delivered.push_back(*msg_id);

    free(buffer);
}

void PerfectLink::log_state(std::ofstream &file)
{
    // std::cout << "loooog" << std::endl;
    for (size_t i = 0; i < broadcasted.size(); ++i)
    {
        file << "b " << broadcasted[i] << "\n";
    }

    for (size_t i = 0; i < delivered.size(); ++i)
    {
        file << "d " << delivered[i].src_id << " " << delivered[i].seq_nr << "\n";
    }
}

void PerfectLink::deliver(int src_id, const char *msg)
{
    if (stop_flag)
    {
        while (true)
        {
        }
    }
    // std::cout << "PL--> delivered " << msg << std::endl;
}
