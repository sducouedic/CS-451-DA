#include "uniform_rel_broadcast.hpp"

// TEMP
#include <iostream>

UniformRelBroadcast::UniformRelBroadcast(const std::vector<Process> *processes, int id,
                                         volatile bool *stop_flag, PerfectLink *pl, BroadcastUnit *upper)
    : BroadcastUnit(processes, id, stop_flag, upper), best_eff_broadcast(nullptr), MIN_ACKS(0)
{
    best_eff_broadcast = new BestEffortBroadcast(processes, id, stop_flag, pl, this);
    MIN_ACKS = (static_cast<int>(processes->size()) / 2) + 1;
}

void UniformRelBroadcast::broadcast(const Message &message)
{
    std::cout << "URB broadcasts (" << message.src_id << "," << message.seq_nr << ")" << std::endl;
    // forwarded.insert(MessageFrom(id, message.seq_nr)); TODO
    MessageFrom msg = MessageFrom(message.src_id, message.seq_nr);
    message_acks.insert(std::pair{msg, std::vector<int>(0)});
    best_eff_broadcast->broadcast(message);
}

void UniformRelBroadcast::send(int dest_id, const Message &message) {}

void UniformRelBroadcast::receive(int src_id, const Message &message)
{
    MessageFrom msg = MessageFrom(message.src_id, message.seq_nr);
    // ignore if already delivered
    for (auto &d : delivered)
    {
        if (d.first == msg.first && d.second == msg.second)
        {
            std::cout << "urb : (" << msg.first << "," << msg.second << ") already delivered " << std::endl;
            return;
        }
    }

    // insert ack and check if has already been forwarded
    bool is_not_in_forwarded = insert_ack(msg, src_id);
    if (is_not_in_forwarded)
    {
        // TODO remove
        // std::cout << "(" << msg.first << "," << msg.second << ")"
        //   << " is not in forwarded" << std::endl;
        best_eff_broadcast->broadcast(message);
    }
    // TODO remove
    // else
    // {
    //     std::cout << "(" << msg.first << "," << msg.second << ")"
    //               << " is already in forwarded" << std::endl;
    // }

    // count number of ACKs
    auto acks = message_acks.find(msg);
    if (acks == message_acks.end())
    {
        std::cerr << "Message must have an entry in list of ACKs" << std::endl;
        exit(EXIT_FAILURE);
    }

    // test if message can now be delivered
    if (can_deliver(msg))
    {
        // important: the src_id is now the id of original sender
        deliver(message.src_id, message);
    }

    /* TODO maybe redundant and not needed
    // search message in forwarded
    auto f = forwarded.find(msg);
    if (f == forwarded.end())
    {
        std::cout << "urb : insert new entry for message " << msg.first << " " << msg.second << " from p " << src_id << std::endl;
        forwarded.insert(msg);
    } */
}

void UniformRelBroadcast::deliver(int src_id, const Message &message)
{
    std::cout << "URB --> delivers (" << src_id << "," << message.seq_nr << "), " << std::endl;

    delivered.push_back(MessageFrom(src_id, message.seq_nr));
    if (upper_layer != nullptr)
    {
        upper_layer->receive(src_id, message);
    }
}

bool UniformRelBroadcast::insert_ack(const MessageFrom &message, int src_id)
{
    int count_msg_acks = 0;

    auto acks = message_acks.find(message);
    bool is_new_message = (acks == message_acks.end());
    if (is_new_message)
    {
        // TODO
        // std::cout << "urb : insert new message in ack (" << message.first << "," << message.second << "), from process " << src_id << std::endl;
        message_acks.insert(std::pair{message, std::vector<int>(1, src_id)});
        count_msg_acks = 1;
    }
    else
    {
        // TODO
        // std::cout << "urb : ack entry already exist, insert new for ack (" << message.first << "," << message.second
        //   << ") : p_id = " << src_id << std::endl;
        bool has_acked = false;
        for (auto &ack : acks->second)
        {
            if (ack == src_id)
            {
                has_acked = true;
            }
        }
        if (!has_acked)
        {
            acks->second.push_back(src_id);
        }
    }

    return is_new_message;
}

bool UniformRelBroadcast::can_deliver(const MessageFrom &message)
{
    auto acks = message_acks.find(message);
    // no ack
    if (acks == message_acks.end())
    {
        return false;
    }

    int count_msg_acks = static_cast<int>(acks->second.size());
    return count_msg_acks >= MIN_ACKS;
}