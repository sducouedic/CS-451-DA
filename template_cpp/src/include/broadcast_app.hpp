#pragma once

#include "perfect_link.hpp"
#include "best_effort_broadcast.hpp"
#include "uniform_rel_broadcast.hpp"
#include "fifo_broadcast.hpp"
#include "local_causal_broadcast.hpp"
#include <fstream>

/// Broadcast configurations
struct Config
{
    int nb_msgs;                          // number of message to send
    std::vector<int> affecting_processes; // processes the current process is affected to

    // used only for perfect link testing
    // int dest_id; // whom to send all those messages
};

/// The main application that broadcasts and delivers messages depending on the configurations
class BroadcastApp
{
public:
    // flag is set in main.cpp when terminal signal arises
    inline volatile static bool stop_flag = false;

    /// Constructor
    BroadcastApp(const Config &configs, const std::vector<Process> &processes, int id, const char *outputPath)
        : id(id), processes(processes), configs(configs), outputPath(outputPath) {}

    /// Start the app
    void start()
    {
        file.open(outputPath);

        // create perfect link and broadcaster
        PerfectLink *perfect_link = new PerfectLink(&processes, id, &stop_flag);
        // broadcaster = new BestEffortBroadcast(&processes, id, &stop_flag, perfect_link);
        // broadcaster = new UniformRelBroadcast(&processes, id, &stop_flag, perfect_link);
        // broadcaster = new FIFOBroadcast(&processes, id, &stop_flag, perfect_link);
        broadcaster = new LocalCausalBroadcast(&processes, id, &configs.affecting_processes, &stop_flag, perfect_link);

        // connect to network
        perfect_link->start_networking();

        // LCB
        char *msg = static_cast<char *>(malloc(MSG_SIZE_LCB + 1));
        for (int i = 0; i < configs.nb_msgs; ++i)
        {
            // build message
            snprintf(msg, MSG_SIZE_LCB, "%d", i + 1);
            msg[MSG_SIZE_LCB] = '\0';
            Message message;
            message.src_id = id;
            message.seq_nr = i + 1;
            message.msg = msg;

            broadcaster->broadcast(message);
        }
        free(msg);

        // FIFO
        // char *msg = static_cast<char *>(malloc(MSG_SIZE_FIFO + 1));
        // for (int i = 0; i < configs.nb_msgs; ++i)
        // {

        //     // build message
        //     snprintf(msg, MSG_SIZE_FIFO, "%d", i + 1);
        //     msg[MSG_SIZE_FIFO] = '\0';
        //     Message message;
        //     message.src_id = id;
        //     message.seq_nr = i + 1;
        //     message.msg = msg;

        //     broadcaster->broadcast(message);
        // }
        // free(msg);

        // LCB
        // if (id == 2)
        // {
        //     sleep(5);
        // }
        // char *msg = static_cast<char *>(malloc(MSG_SIZE_LCB + 1));
        // for (int i = 0; i < 5; ++i)
        // {
        //     if (id == 3)
        //     {
        //         break;
        //     }
        //     // build message
        //     snprintf(msg, MSG_SIZE_LCB, "%d", i + 1);
        //     msg[MSG_SIZE_LCB] = '\0';
        //     Message message;
        //     message.src_id = id;
        //     message.seq_nr = i + 1;
        //     message.msg = msg;

        //     broadcaster->broadcast(message);
        // }
        // sleep(15);
        // for (int i = 5; i < configs.nb_msgs; ++i)
        // {
        //     if (id == 3)
        //     {
        //         break;
        //     }
        //     // build message
        //     snprintf(msg, MSG_SIZE_LCB, "%d", i + 1);
        //     msg[MSG_SIZE_LCB] = '\0';
        //     Message message;
        //     message.src_id = id;
        //     message.seq_nr = i + 1;
        //     message.msg = msg;

        //     broadcaster->broadcast(message);
        // }
        // free(msg);

        // FIFO
        // if (id == 2)
        // {
        //     sleep(5);
        // }
        // char *msg = static_cast<char *>(malloc(MSG_SIZE_FIFO + 1));
        // for (int i = 0; i < 5; ++i)
        // {
        //     if (id == 3)
        //     {
        //         break;
        //     }
        //     // build message
        //     snprintf(msg, MSG_SIZE_LCB, "%d", i + 1);
        //     msg[MSG_SIZE_LCB] = '\0';
        //     Message message;
        //     message.src_id = id;
        //     message.seq_nr = i + 1;
        //     message.msg = msg;

        //     broadcaster->broadcast(message);
        // }
        // sleep(15);
        // for (int i = 5; i < configs.nb_msgs; ++i)
        // {
        //     if (id == 3)
        //     {
        //         break;
        //     }
        //     // build message
        //     snprintf(msg, MSG_SIZE_FIFO, "%d", i + 1);
        //     msg[MSG_SIZE_FIFO] = '\0';
        //     Message message;
        //     message.src_id = id;
        //     message.seq_nr = i + 1;
        //     message.msg = msg;

        //     broadcaster->broadcast(message);
        // }
        // free(msg);
    }

    /// Log current state in file
    void log_state()
    {
        // log current (frozen) state
        if (broadcaster != nullptr)
        {
            broadcaster->log_state(file);
        }
        file.close();
    }

private:
    BroadcastUnit *broadcaster = nullptr; // broadcaster object

    int id;                         // current process id
    std::vector<Process> processes; // list of all the processes

    Config configs; // broadcast configs
    const char *outputPath;
    std::ofstream file;
};