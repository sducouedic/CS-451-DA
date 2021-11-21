#include "perfect_link.hpp"
#include <limits.h>
//TEMP
#include <iostream>

#define MSG_SIZE_PL (MSG_SIZE_TCP - SRC_ID_SIZE - SEQ_SIZE)

PerfectLink::PerfectLink(const std::vector<Process> *processes, int id, volatile bool *stop_flag, BroadcastUnit *upper)
    : NetworkUnit(processes, id, stop_flag, upper), pl_seq_nr(0) {}

void PerfectLink::set_upper(BroadcastUnit *upper)
{
    upper_layer = upper;
}

void PerfectLink::send(int dest_id, const Message &message)
{
    ++pl_seq_nr;

    // build netwok message
    Message network_msg;
    network_msg.src_id = id;
    network_msg.seq_nr = pl_seq_nr;
    network_msg.msg = static_cast<char *>(malloc(MSG_SIZE_TCP + 1));
    encode_message(message, network_msg.msg, MSG_SIZE_PL);

    network_send(dest_id, network_msg);

    free(network_msg.msg);

    // not needed for milestones 2 and 3
    // broadcasted.push_back(message.seq_nr);
}

void PerfectLink::receive(int src_id, const Message &network_msg)
{
    for (auto &d : delivered)
    {
        if (d.first == src_id && d.second == network_msg.seq_nr)
        {
            return;
        }
    }

    // deliver message
    deliver(src_id, network_msg);
}

void PerfectLink::log_state(std::ofstream &file)
{
    /* PL Do not log anything in milestone 2 and 3
    for (size_t i = 0; i < broadcasted.size(); ++i)
    {
        file << "b " << broadcasted[i] << "\n";
    }

    for (size_t i = 0; i < delivered.size(); ++i)
    {
        file << "d " << delivered[i].first << " " << delivered[i].second << "\n";
    }
    */
}

void PerfectLink::deliver(int src_id, const Message &network_msg)
{
    // add the message to the list of delivered messages
    // TODO remove stop_flag
    if (!stop_flag)
    {
        // extract inner message
        Message message;
        message.msg = static_cast<char *>(malloc(MSG_SIZE_PL + 1));
        extract_message(message, network_msg.msg, MSG_SIZE_PL);

        delivered.push_back(MessageFrom(src_id, network_msg.seq_nr));

        if (upper_layer != nullptr)
        {
            // TODO
            // std::cout << "PL delivers (" << src_id << "," << network_msg.seq_nr << ")" << std::endl;
            upper_layer->receive(src_id, message);
        }
        free(message.msg);
    }
}
