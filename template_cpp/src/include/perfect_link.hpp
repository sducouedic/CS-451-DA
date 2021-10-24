#pragma once

#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <list>

#include "broadcast_unit.hpp"
#include "parser.hpp"

#define MAXLINE 1024
#define SEQ_SIZE 4
#define ACK_SIZE 1
#define MSG_START (SEQ_SIZE + ACK_SIZE)
#define MSG_SIZE (MAXLINE - MSG_START) - 1

// Define an acknowledgement from a host of a msg
struct ACK {
    int host_id;
    int seq_nr;
};

class PerfectLink: public BroadcastUnit {
public:

    /// Class constructor : setup the list of hosts addresses then open and bind a socket
    PerfectLink(std::vector<Parser::Host>& hosts, int locId, volatile bool& stop_flag);

    /// Class desctructor : close the socket
    ~PerfectLink();

    /// TODO Send a message to a recipient host (@see BroadcastUnit)
    void send(int dest_id, int seq_nr, const char* msg) override;

    /// TODO Receive a message from a source host (@see BroadcastUnit)
    void receive(int src_id, int seq_nr, const char* msg) override;

    /// Close the socket
    void pl_close();

protected:

    /// TODO Deliver a message from a source host to the upper layer (@see BroadcastUnit)
    void deliver(int src_id, int seq_nr, const char* msg);


// private attributess
private:

    int sockfd;    // socket number

    std::vector<sockaddr_in> sockaddrs; // socket addresses

    std::list<ACK> lacking_acks; // ACKs we are still waiting for

// private methods
private:

    // listen to socket (either for acks or new msg)
    void socket_receive();

    // send message through socket
    void socket_send(sockaddr_in* addr, int seq_nr, const char* msg);

    // handling ack from a host for a given msg
    void handle_ack(ACK ack);

    // initialise the vector of hosts sockaddrs
    void setup_sockaddrs(std::vector<Parser::Host>& hosts);

    // get the sockaddr corresponding to a host id
    sockaddr_in* get_sockaddr_from_id(int id);

    // get the host id corresponding to a sockaddr
    int get_id_from_sockaddr(sockaddr_in* addr);
};

