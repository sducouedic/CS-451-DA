#pragma once

#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <list>

#define MAXLINE 64
#define ACK_SIZE 1
#define SEQ_SIZE 4
#define MSG_START (SEQ_SIZE + ACK_SIZE)
#define MSG_SIZE (MAXLINE - MSG_START) - 1

// Define an acknowledgement from a host for a msg
struct ACK
{
    const sockaddr_in *addr; // host that acknowledged
    int seq_nr;              // sequence number of acknowledged message
    char *msg = nullptr;     // acknowledged message

    // two acknowledgments are the same if same host and seq_nr
    bool isEqual(const ACK &a) const
    {
        return a.addr == addr && a.seq_nr == seq_nr;
    }
};

// forward declaration because of interdependencies
class NetworkUnit;

/// A class that approximates TCP (or stubborn link in some sense)
/// Continuously resends a message until it receives an acknowledgment
class ApproxTCP
{

public:
    /// Class Constructor: setup socakaddr and upper layer instance
    ApproxTCP(const sockaddr_in *host_addr, NetworkUnit *upper_layer = nullptr);

    // start listenning to socket and send messages
    void start();

    // send message through the socket
    void socket_send(const sockaddr_in *dest, int seq_nr, const char *msg);

    void set_network_unit(NetworkUnit *unit)
    {
        upper_layer = unit;
    }

private:
    const sockaddr_in *addr; // host address

    int sockfd; // socket file descriptor

    NetworkUnit *upper_layer; // perfectlink to whom deliver msg

    std::list<ACK> lacking_acks; // ACKs we are still waiting for

private:
    // listen to socket (either for acks or new msg)
    void socket_polling();

    // continuously send all the messages
    void socket_pushing();

    // received a message from a host
    void socket_receive(const sockaddr_in *src, int seq_nr, const char *msg);

    // handling ack from a host for a given msg
    void handle_ack(ACK &ack);

    // create a socket file descriptor and bind with server address
    int create_bind_socket();

    // build udp packet given ack, seq_nr and message
    void build_udp_packet(bool is_ack, int seq_nr, const char *msg, char *buffer);

    // extract information from udp packet
    void extract_from_udp_packet(bool &is_ack, int &seq_nr, char *msg, const char *udp_packet);
};
