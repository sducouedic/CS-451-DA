#include "best_effort_broadcast.hpp"

// TEMP
#include <iostream>

BestEffortBroadcast::BestEffortBroadcast(const std::vector<Process> *processes, int id,
                                         volatile bool *stop_flag, PerfectLink *pl,
                                         BroadcastUnit *upper)
    : BroadcastUnit(processes, id, stop_flag), perfect_link(pl), upper_layer(upper) {}

void BestEffortBroadcast::broadcast(const Message &message)
{
    for (auto &process : processes)
    {
        send(process.id, message);
    }
}

void BestEffortBroadcast::send(int dest_id, const Message &message)
{
    perfect_link->send(dest_id, message);
}

void BestEffortBroadcast::receive(int src_id, const Message &message)
{
    deliver(src_id, message);
}

void BestEffortBroadcast::deliver(int src_id, const Message &message)
{
    if (upper_layer != nullptr)
    {
        upper_layer->receive(src_id, message);
    }
    // TEMP
    std::cout << "final deliver" << std::endl;
}