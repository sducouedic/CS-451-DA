#include "perfect_link.hpp"
#include <limits.h>
//TEMP
#include <iostream>

PerfectLink::PerfectLink(const std::vector<Process> &processes, int id, volatile bool *stop_flag)
    : NetworkUnit(processes), id(id), stop_flag(*stop_flag)
{
    this->tcp = new ApproxTCP(sockaddr_from_id(id), this);
}

void PerfectLink::send(int dest_id, int seq_nr, const char *msg)
{
    std::cout << "PL send" << std::endl;
    tcp->socket_send(sockaddr_from_id(dest_id), seq_nr, msg);
    broadcasted.push_back(seq_nr);
}

void PerfectLink::receive(int src_id, int seq_nr, const char *msg)
{
    //TEMP
    std::cout << "PL --> src: " << src_id << " , seq: " << seq_nr
              << ", msg: " << msg << std::endl;

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

    deliver(src_id, seq_nr, buffer);
    free(buffer);
}

void PerfectLink::log_state(std::ofstream &file)
{
    for (size_t i = 0; i < broadcasted.size(); ++i)
    {
        file << "b " << broadcasted[i] << "\n";
    }

    for (size_t i = 0; i < delivered.size(); ++i)
    {
        file << "d " << delivered[i].src_id << " " << delivered[i].seq_nr << "\n";
    }
}

void PerfectLink::deliver(int src_id, int seq_nr, const char *msg)
{
    MessageId *msg_id = new MessageId;
    msg_id->src_id = src_id;
    msg_id->seq_nr = seq_nr;

    if (!stop_flag)
    {
        delivered.push_back(*msg_id);
        std::cout << "PL--> delivered " << msg << std::endl;
    }
}
