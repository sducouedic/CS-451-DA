#include "fifo_broadcast.hpp"

// TEMP
#include <iostream>

FIFOBroadcast::FIFOBroadcast(const std::vector<Process> *processes, int id,
                             volatile bool *stop_flag, PerfectLink *pl, BroadcastUnit *upper)
    : BroadcastUnit(processes, id, stop_flag, upper), uniform_rel_broadcast(nullptr)
{
    uniform_rel_broadcast = new UniformRelBroadcast(processes, id, stop_flag, pl, this);

    int nb_processes = static_cast<int>(processes->size());
    // pending: empty list for all the processes
    for (int i = 0; i < nb_processes; ++i)
    {
        pendings.push_back(std::list<Message *>(0));
    }
}

void FIFOBroadcast::broadcast(const Message &message)
{
    if (!stop_flag)
    {
        std::cout << "FIFO broadcasts (" << message.src_id << "," << message.seq_nr << ")" << std::endl;
        uniform_rel_broadcast->broadcast(message);

        // log the broadcast event
        Event e = {'b', id, message.seq_nr};
        events.push(e);
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
    std::list<Message *> &src_pendings = pendings[src_id - 1];
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
        std::cerr << "was not able to update pendings" << msg->seq_nr << std::endl;
        exit(EXIT_FAILURE);
    }

    // call deliver_pending
    deliver_pending(src_id);
}

void FIFOBroadcast::deliver(int src_id, const Message &message)
{
    std::cout << "FIFO --> delivers (" << src_id << "," << message.seq_nr << ")" << std::endl;

    // log the delivery event
    Event e = {'d', src_id, message.seq_nr};
    events.push(e);

    if (upper_layer != nullptr)
    {
        upper_layer->receive(src_id, message);
    }
}

void FIFOBroadcast::deliver_pending(int src_id)
{
    std::list<Message *> &src_pendings = pendings[src_id - 1];

    bool prev_was_delivered = true;
    while (prev_was_delivered)
    {
        Message *first = *(src_pendings.begin());
        if (first == nullptr)
        {
            break;
        }

        if (first->seq_nr == vector_clock[src_id - 1] + 1 and !stop_flag)
        {
            // remove from pendings
            src_pendings.pop_front();

            // deliver message
            deliver(src_id, *first);

            // free memory
            free(first->msg);
            delete (first);

            // update vector clock
            vector_clock[src_id - 1] += 1;
        }
        else
        {
            prev_was_delivered = false;
        }
    }
}

void FIFOBroadcast::log_state(std::ofstream &file)
{
    while (events.has_next())
    {
        Event e = events.get_next();
        if (e.type == 'b')
        {
            file << "b " << e.seq_nr << "\n";
        }
        else if (e.type == 'd')
        {
            file << "d " << e.src_id << " " << e.seq_nr << "\n";
        }
        else
        {
            std::cerr << "Unexptected event log" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}