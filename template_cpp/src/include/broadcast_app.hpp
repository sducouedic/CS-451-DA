#pragma once

#include "perfect_link.hpp"
#include <fstream>

/// Broadcast configurations
struct Config
{
    int nb_msgs; // number of message to send
    int dest_id; // who to send all those messages
};

/// The main application that broadcasts and delivers messages depending on the configurations
class BroadcastApp
{
public:
    // flag is set in main.cpp when terminal signal arises
    inline volatile static bool stop_flag = false;

    /// Constructor
    BroadcastApp(const Config &configs, const std::vector<Process> &processes, int id, const char *outputPath)
        : configs(configs), processes(processes), id(id), outputPath(outputPath)
    {
        pf = new PerfectLink(processes, id, &stop_flag);
    }

    /// Start the app
    void start()
    {
        file.open(outputPath);

        // connect perfectlink to the network
        pf->start_networking();

        // send messages
        if (id != configs.dest_id)
        {
            char *msg = static_cast<char *>(malloc(MSG_SIZE));
            for (int i = 0; i < configs.nb_msgs; ++i)
            {
                snprintf(msg, MSG_SIZE, "%d", i + 1);
                pf->send(configs.dest_id, i + 1, msg);
            }
        }
    }

    /// Log current state in file
    void log_state()
    {
        pf->log_state(file);
        file.close();
    }

private:
    PerfectLink *pf;

    Config configs;
    std::vector<Process> processes;
    int id;

    const char *outputPath;
    std::ofstream file;
};