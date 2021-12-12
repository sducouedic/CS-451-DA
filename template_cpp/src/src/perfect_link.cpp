#include "perfect_link.hpp"
#include <limits.h>
// TEMP
#include <iostream>

// TODO remove
#define MSG_SIZE_LCB (MSG_SIZE_PL - VECTOR_CLOCK_SIZE)

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
    encode_message_to_chars(message, network_msg.msg);

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
    // PL Do not log anything in milestone 2 and 3
    // for (size_t i = 0; i < broadcasted.size(); ++i)
    // {
    //     file << "b " << broadcasted[i] << "\n";
    // }

    for (size_t i = 0; i < delivered.size(); ++i)
    {
        file << "d " << delivered[i].first << " " << delivered[i].second << "\n";
    }
}

void PerfectLink::deliver(int src_id, const Message &network_msg)
{
    // extract inner message
    Message message;
    message.msg = static_cast<char *>(malloc(MSG_SIZE_PL + 1));
    extract_message_from_chars(message, network_msg.msg);

    // add the message to the list of delivered messages
    delivered.push_back(MessageFrom(src_id, network_msg.seq_nr));

    if (upper_layer != nullptr)
    {
        // TODO
        // std::cout << "PL delivers (" << message.src_id << "," << message.seq_nr << ")" << std::endl;
        upper_layer->receive(src_id, message);
    }
    free(message.msg);
}

void PerfectLink::encode_message_to_chars(const Message &message, char *buffer)
{
    // set src_id
    char src_id = static_cast<char>(message.src_id);
    buffer[0] = src_id;

    // set seq_nr
    int seq_nr = message.seq_nr;
    memcpy(buffer + SRC_ID_SIZE, reinterpret_cast<char *>(&seq_nr), SEQ_SIZE);

    // set message
    memcpy(buffer + SEQ_SIZE + SRC_ID_SIZE, message.msg, MSG_SIZE_PL);
    buffer[MSG_SIZE_TCP] = '\0';

    if (message.seq_nr != 0 and buffer[SRC_ID_SIZE] == 0 and buffer[SRC_ID_SIZE + 1] == 0 and buffer[SRC_ID_SIZE + 2] == 0 and buffer[SRC_ID_SIZE + 3] == 0)
    {
        std::cerr << message.seq_nr << " encoding error" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void PerfectLink::extract_message_from_chars(Message &message, const char *buffer)
{
    // extract src_id
    int src_id = static_cast<int>(buffer[0]);
    if (src_id == CHAR_MIN)
    {
        src_id = CHAR_MAX + 1;
    }
    message.src_id = src_id;

    // extract seq_nr
    memcpy(&(message.seq_nr), buffer + SRC_ID_SIZE, SEQ_SIZE);

    // extract msg
    char *msg = static_cast<char *>(malloc(MSG_SIZE_PL + 1));
    memcpy(msg, buffer + SRC_ID_SIZE + SEQ_SIZE, MSG_SIZE_PL);
    msg[MSG_SIZE_PL] = '\0';
    message.msg = msg;
}
