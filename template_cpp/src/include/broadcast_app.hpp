#pragma once

#include "perfect_link.hpp"
#include <fstream>

struct Config
{
    int nb_msgs;
    int dest_id;
};

class BroadcastApp
{
public:
    inline volatile static bool stop_flag = false;

    BroadcastApp(const Config &configs, const std::vector<Process> &processes, int id, const char *outputPath)
        : configs(configs), processes(processes), id(id), outputPath(outputPath)
    {
        pf = new PerfectLink(processes, id, &stop_flag);
    }

    void start()
    {
        file.open(outputPath);

        pf->getTCP()->start();
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

    void log_state()
    {
        pf->log_state(file);
        file.close();
    }

private:
    Config configs;

    std::vector<Process> processes;

    int id;

    const char *outputPath;

    std::ofstream file;

    PerfectLink *pf;
};