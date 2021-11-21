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
    // std::cout << "URB broadcasts (" << message.src_id << "," << message.seq_nr << ")" << std::endl;
    last_broadcasted_seq_num = message.seq_nr;

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
            return;
        }
    }

    // insert ack and check if has already been forwarded
    bool is_not_in_forwarded = insert_ack(msg, src_id);
    if (is_not_in_forwarded)
    {
        best_eff_broadcast->broadcast(message);
    }

    // test if message can now be delivered
    if (can_deliver(msg))
    {
        // important: the src_id is now the id of original sender
        deliver(message.src_id, message);
    }
}

void UniformRelBroadcast::deliver(int src_id, const Message &message)
{
    // std::cout << "URB --> delivers (" << src_id << "," << message.seq_nr << ")" << std::endl;

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
        message_acks.insert(std::pair{message, std::vector<int>(1, src_id)});
        count_msg_acks = 1;
    }
    else
    {
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

void UniformRelBroadcast::log_state(std::ofstream &file)
{
    // for (size_t i = 1; static_cast<int>(i) <= last_broadcasted_seq_num; ++i)
    // {
    //     file << "b " << i << "\n";
    // }

    for (size_t i = 0; i < delivered.size(); ++i)
    {
        file << "d " << delivered[i].first << " " << delivered[i].second << "\n";
    }
}