#include "fifo_broadcast.hpp"

// TEMP
#include <iostream>

FIFOBroadcast::FIFOBroadcast(const std::vector<Process> *processes, int id,
                             volatile bool *stop_flag, PerfectLink *pl, BroadcastUnit *upper)
    : BroadcastUnit(processes, id, stop_flag, upper), uniform_rel_broadcast(nullptr)
{
    uniform_rel_broadcast = new UniformRelBroadcast(processes, id, stop_flag, pl, this);

    // vector_cloacks: associate 0 to all processes
    int nb_processes = static_cast<int>(processes->size());
    for (int i = 1; i <= nb_processes; ++i)
    {
        vector_clocks.insert(std::pair{i, 0});
    }

    // pending: empty list for all the processes
    for (int i = 1; i <= nb_processes; ++i)
    {
        pendings.insert(std::pair{i, std::list<Message *>(0)});
    }
}

void FIFOBroadcast::broadcast(const Message &message)
{
    if (!stop_flag)
    {
        // std::cout << "FIFO broadcasts (" << message.src_id << "," << message.seq_nr << ")" << std::endl;
        if (message.seq_nr <= last_broadcasted_seq_num)
        {
            std::cerr << "sequence number must be incremental" << std::endl;
            exit(EXIT_FAILURE);
        }
        last_broadcasted_seq_num = message.seq_nr;
        uniform_rel_broadcast->broadcast(message);
    }
}

void FIFOBroadcast::send(int dest_id, const Message &message) {}

void FIFOBroadcast::receive(int src_id, const Message &message)
{
    // std::cout << "FIFO receives (" << src_id << "," << message.seq_nr << ")" << std::endl;

    // Build copy of message
    Message *msg = new Message;
    msg->src_id = message.src_id;
    msg->seq_nr = message.seq_nr;
    msg->msg = static_cast<char *>(malloc(MSG_SIZE_FIFO + 1));
    strncpy(msg->msg, message.msg, MSG_SIZE_FIFO);
    msg->msg[MSG_SIZE_FIFO] = '\0';

    // update pending
    bool updated = false;
    std::list<Message *> &src_pendings = pendings_of_src(src_id);
    if (src_pendings.empty() || msg->seq_nr > src_pendings.back()->seq_nr)
    {
        src_pendings.push_back(msg);
        updated = true;
    }
    else
    {
        for (auto it = src_pendings.begin(); it != src_pendings.end(); ++it)
        {
            if (msg->seq_nr < (*it)->seq_nr)
            {
                src_pendings.insert(it, msg);
                updated = true;
                break;
            }
        }
    }
    if (!updated)
    {
        std::cerr << "was not able to update " << msg->seq_nr << std::endl;
        exit(EXIT_FAILURE);
    }

    // call deliver_pending
    deliver_pending(src_id);
}

void FIFOBroadcast::deliver(int src_id, const Message &message)
{
    std::cout << "FIFO --> delivers (" << src_id << "," << message.seq_nr << ")" << std::endl;

    delivered.push_back(MessageFrom(src_id, message.seq_nr));
    if (upper_layer != nullptr)
    {
        upper_layer->receive(src_id, message);
    }
}

void FIFOBroadcast::deliver_pending(int src_id)
{
    auto vc = vector_clocks.find(src_id);
    if (vc == vector_clocks.end())
    {
        std::cerr << "Vector clock must have an entry for each process " << std::endl;
        exit(EXIT_FAILURE);
    }

    std::list<Message *> &src_pendings = pendings_of_src(src_id);

    bool prev_was_delivered = true;
    while (prev_was_delivered)
    {
        Message *first = *(src_pendings.begin());
        if (first == nullptr)
        {
            break;
        }

        if (first->seq_nr == vc->second + 1)
        {
            if (!stop_flag)
            {
                // remove from pendings
                src_pendings.pop_front();

                // deliver message
                deliver(src_id, *first);

                // free memory
                free(first->msg);
                delete (first);

                // update vc
                vc->second = vc->second + 1;
            }
        }
        else
        {
            prev_was_delivered = false;
        }
    }
}

std::list<Message *> &FIFOBroadcast::pendings_of_src(int src_id)
{
    auto pendings_tmp = pendings.find(src_id);
    if (pendings_tmp == pendings.end())
    {
        std::cerr << "Pendings must have an entry for each process " << std::endl;
        exit(EXIT_FAILURE);
    }
    return pendings_tmp->second;
}

void FIFOBroadcast::log_state(std::ofstream &file)
{
    // for (size_t i = 1; static_cast<int>(i) <= last_broadcasted_seq_num; ++i)
    // {
    //     file << "b " << i << "\n";
    // }

    for (size_t i = 1; i <= pendings.size(); ++i)
    {
        auto p = pendings_of_src(static_cast<int>(i));
        std::cout << "process " << i << ":    ";
        for (auto &j : p)
        {
            std::cout << j->seq_nr << " ";
        }
        std::cout << std::endl
                  << std::endl;
    }

    for (size_t i = 0; i < delivered.size(); ++i)
    {
        file << "d " << delivered[i].first << " " << delivered[i].second << "\n";
    }

    file << "\n\n\n\n";
    uniform_rel_broadcast->log_state(file);
}