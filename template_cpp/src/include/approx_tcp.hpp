#pragma once

#include "broadcast_unit.hpp"

#include <thread>
#include <list>

#define ACK_SIZE 1
#define MAXLINE 577
#define MAX_MESSAGES_IN_PACKET 7
#define MSG_START_TCP (ACK_SIZE + SRC_ID_SIZE + SEQ_SIZE)
#define MSG_SIZE_TCP (MAXLINE - MSG_START_TCP) - 1

/// Defines an acknowledgement from a host for a msg
struct ACK
{
    const sockaddr_in *addr = nullptr; // host that acknowledged
    Message message;                   // acknowledged message

    // two acknowledgments are the same if same host and seq_nr
    bool isEqual(const ACK &a) const
    {
        return a.addr == addr && a.message.seq_nr == message.seq_nr;
    }
};

// Forward declaration because of interdependencies
class NetworkUnit;

/// A class that approximates TCP (or stubborn link in some sense)
/// Continuously resends a message until it receives an acknowledgment
class ApproxTCP
{

public:
    /// Class Constructor: setup sockaddr and upper layer instance
    ApproxTCP(const sockaddr_in *host_addr, NetworkUnit *upper_layer = nullptr);

    /// Class destructor: default
    virtual ~ApproxTCP() = default;

    /// starts listenning to socket and sending messages
    void start();

    /// send message through the socket
    void socket_send(const sockaddr_in *dest, const Message &tcp_msg);

    /// set the upper layer
    void set_network_unit(NetworkUnit *unit)
    {
        upper_layer = unit;
    }

private:
    const sockaddr_in *addr;     // host address
    int sockfd;                  // socket file descriptor
    NetworkUnit *upper_layer;    // network unit to whom to deliver msg
    std::list<ACK> lacking_acks; // ACKs we are still waiting for
    int tcp_seq_nr;

private:
    // listen to socket (either for acks or new msg)
    void socket_polling();

    // continuously send all the messages
    void socket_pushing();

    // received a message from a host
    void socket_receive(const sockaddr_in *src, const Message &tcp_msg);

    // handling ack from a host for a given msg
    void handle_ack(ACK &ack);

    // create a socket file descriptor and bind with server address
    int create_bind_socket();

    // build udp packet given ack, seq_nr and message
    void build_udp_packet(bool is_ack, const Message &tcp_msg, char *buffer);

    // extract information from udp packet
    void extract_from_udp_packet(bool &is_ack, Message &tcp_msg, const char *udp_packet);
};
