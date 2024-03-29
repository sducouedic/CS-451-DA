#include "local_causal_broadcast.hpp"

// TEMP
#include <iostream>

LocalCausalBroadcast::LocalCausalBroadcast(const std::vector<Process> *processes, int id,
                                           const std::vector<int> *affecting_processes, volatile bool *stop_flag,
                                           PerfectLink *pl, BroadcastUnit *upper)
    : BroadcastUnit(processes, id, stop_flag, upper), uniform_rel_broadcast(nullptr), vc_broadcasted(0),
      affecting_processes(*affecting_processes)
{
    uniform_rel_broadcast = new UniformRelBroadcast(processes, id, stop_flag, pl, this);

    int nb_processes = static_cast<int>(processes->size());
    // pending: empty list for all the processes
    for (int i = 0; i < nb_processes; ++i)
    {
        pendings.push_back(std::list<LCBMessage *>(0));
    }

    // fill vector_clock with zeros
    for (int i = 0; i < MAX_PROCESSES; ++i)
    {
        vector_clock[i] = 0;
    }
}

void LocalCausalBroadcast::broadcast(const Message &message)
{
    if (!stop_flag)
    {
        std::cout << "LCB --> broadcast (" << message.seq_nr << ")" << std::endl;

        // create new message
        Message new_msg;
        new_msg.src_id = message.src_id;
        new_msg.seq_nr = message.seq_nr;
        append_vector_clock(new_msg, message.msg);

        // log the broadcast event
        Event e = {'b', id, message.seq_nr};
        events.push(e);

        // broadcast
        uniform_rel_broadcast->broadcast(new_msg);

        // update vc_broadcasted
        ++vc_broadcasted;

        // free memory
        free(new_msg.msg);
    }
}

void LocalCausalBroadcast::send(int dest_id, const Message &message) {}

void LocalCausalBroadcast::receive(int src_id, const Message &message)
{
    // Build copy of message
    LCBMessage *msg = new LCBMessage;
    msg->src_id = message.src_id;
    msg->seq_nr = message.seq_nr;
    msg->msg = static_cast<char *>(malloc(MSG_SIZE_LCB + 1));
    strncpy(msg->msg, message.msg + VECTOR_CLOCK_SIZE, MSG_SIZE_LCB);
    msg->msg[MSG_SIZE_LCB] = '\0';
    set_message_vector_clock(*msg, message.msg);

    // update pending
    bool updated = false;

    std::list<LCBMessage *> &src_pendings = pendings[src_id - 1];

    if (src_pendings.empty() || msg->vector_clock[src_id - 1] > src_pendings.back()->vector_clock[src_id - 1])
    {
        src_pendings.push_back(msg);
        updated = true;
    }
    else
    {
        for (auto it = src_pendings.begin(); it != src_pendings.end(); ++it)
        {
            if (msg->vector_clock[src_id - 1] < (*it)->vector_clock[src_id - 1])
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
    deliver_pending();
}

void LocalCausalBroadcast::deliver(int src_id, const Message &message)
{
    std::cout << "LCB --> delivers (" << src_id << "," << message.seq_nr << ")" << std::endl;

    // log the delivery event
    Event e = {'d', src_id, message.seq_nr};
    events.push(e);

    if (upper_layer != nullptr)
    {
        upper_layer->receive(src_id, message);
    }
}

void LocalCausalBroadcast::deliver_pending()
{
    bool at_least_one_delivered = true;
    while (at_least_one_delivered)
    {
        at_least_one_delivered = false;
        for (auto &src_pendings : pendings)
        {
            bool prev_was_delivered = true;
            while (prev_was_delivered)
            {
                LCBMessage *first = *(src_pendings.begin());
                if (first == nullptr)
                {
                    break;
                }

                bool is_all_higher = true;
                for (int i = 0; i < MAX_PROCESSES; ++i)
                {
                    if (vector_clock[i] < first->vector_clock[i])
                    {
                        is_all_higher = false;
                        break;
                    }
                }

                if (is_all_higher and !stop_flag)
                {
                    at_least_one_delivered = true;

                    // remove from pendings
                    src_pendings.pop_front();

                    // deliver message
                    deliver(first->src_id, *first);

                    // update vector clock
                    ++vector_clock[first->src_id - 1];

                    // free memory
                    free(first->msg);
                    delete (first);
                }
                else
                {
                    prev_was_delivered = false;
                }
            }
        }
    }
}

void LocalCausalBroadcast::log_state(std::ofstream &file)
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

void LocalCausalBroadcast::append_vector_clock(Message &message, const char *msg)
{
    char *buffer = static_cast<char *>(malloc(VECTOR_CLOCK_SIZE + MSG_SIZE_LCB + 1));

    // set zero for all process
    int zero = 0;
    for (int i = 0; i < MAX_PROCESSES; ++i)
    {
        memcpy(buffer + i * sizeof(int), reinterpret_cast<char *>(&zero), sizeof(int));
    }

    // set value of vector clock for affecting processes
    for (auto &p_id : affecting_processes)
    {
        memcpy(buffer + (p_id - 1) * sizeof(int), reinterpret_cast<char *>(&vector_clock[p_id - 1]), sizeof(int));
    }

    // for current process we need to use vc_broadcasted instead
    memcpy(buffer + (id - 1) * sizeof(int), reinterpret_cast<char *>(&vc_broadcasted), sizeof(int));

    strncpy(buffer + VECTOR_CLOCK_SIZE, msg, MSG_SIZE_LCB);
    buffer[VECTOR_CLOCK_SIZE + MSG_SIZE_LCB] = '\0';
    message.msg = buffer;
}

void LocalCausalBroadcast::set_message_vector_clock(LCBMessage &lcb_message, const char *buffer)
{
    for (int i = 0; i < MAX_PROCESSES; ++i)
    {
        memcpy(&(lcb_message.vector_clock[i]), buffer + i * sizeof(int), sizeof(int));
    }
}