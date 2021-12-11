#pragma once

#include "network_unit.hpp"

#define MSG_SIZE_PL (MSG_SIZE_TCP - SRC_ID_SIZE - SEQ_SIZE)

/// An implementation of a perfect link
class PerfectLink : public NetworkUnit
{
public:
    /// Class constructor
    PerfectLink(const std::vector<Process> *processes, int id,
                volatile bool *stop_flag, BroadcastUnit *upper_layer = nullptr);

    /// Class destructor: default
    virtual ~PerfectLink() = default;

    /// Set upper layer
    void set_upper(BroadcastUnit *upper_layer);

    /// (@see BroadcastUnit) Send a message to a recipient host
    void send(int dest_id, const Message &message) override;

    /// (@see BroadcastUnit) Receive a message from a source host
    void receive(int src_id, const Message &network_msg) override;

    /// (@see BroadcastUnit) Write current (frozen) state into output file
    void log_state(std::ofstream &file) override;

protected:
    /// (@see BroadcastUnit) Deliver a message from a source host to the upper layer
    void deliver(int src_id, const Message &network_msg) override;

private:
    int pl_seq_nr;
    std::vector<MessageFrom> delivered; // list of delivered messages (src_id, pl_seq_nr)

    // not needed for milestones 2 and 3
    // std::vector<int> broadcasted;       // list of broadcasted messages

private:
    /// Encode a message into a sequence of characters
    static void encode_message_to_chars(const Message &message, char *buffer, int buffer_size);

    /// Extract a message from a sequence of characters
    static void extract_message_from_chars(Message &message, const char *buffer, int msg_size);
};
