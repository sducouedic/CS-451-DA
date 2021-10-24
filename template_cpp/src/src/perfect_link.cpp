#include "perfect_link.hpp"
#include <limits.h>

PerfectLink::PerfectLink(std::vector<Parser::Host>& hosts, int locId, volatile bool& stop_flag) {

    // build sockaddrs
    setup_sockaddrs(hosts);

    char buffer[1024];
    char *hello = "Hello from server";
    sockaddr_in* addr;
    addr = get_sockaddr_from_id(locId);

    sockaddr_in* servaddr;
    sockaddr_in* cliaddr;
    servaddr = get_sockaddr_from_id(1);
    cliaddr = get_sockaddr_from_id(2);

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        std::cerr << "socket creation failed" << std::flush;
        exit(EXIT_FAILURE);
    }

    // bind the socket with local address
    if ( bind(sockfd, reinterpret_cast<const sockaddr*>(addr), sizeof(*addr)) < 0)
    {
        std::cerr << "bind failed" << std::flush;
        exit(EXIT_FAILURE);
    }

    // listen for incoming socket messages
    std::thread recv_t(&PerfectLink::socket_receive, this);

    // detach the thread
    recv_t.detach();

    if(locId != 1) {
        send(1, 1028, "hello");
    }
}

PerfectLink::~PerfectLink() {
    pl_close();
}

void PerfectLink::send(int dest_id, int seq_nr, const char* msg) {
    //TODO remove
    std::cout << "send message " << msg << std::endl;

    char buffer[MAXLINE];

    // set ack to false
    buffer[0] = 0;

    // set seq_nr
    memcpy(buffer+1, reinterpret_cast<char*>(&seq_nr), sizeof(int));
    std::cout << "seq_nr: ";
    for(int i = 0; i < 4; i++) {
        std::cout << static_cast<int>(buffer[1+i] ) << " "; 
    }

    // set message
    strncpy(buffer+MSG_START, msg, MSG_SIZE);
    buffer[MAXLINE - 1] = '\0';

    sockaddr_in* sockaddr = get_sockaddr_from_id(dest_id);
    socket_send(sockaddr, seq_nr, buffer);
}

// TODO
void PerfectLink::receive(int src_id, int seq_nr, const char* msg) {
    // TODO remove
    std::cout << msg << std::endl;
}

// TODO
void PerfectLink::deliver(int src_id, int seq_nr, const char* msg) {

}

void PerfectLink::pl_close() {
    close(sockfd);
}

void PerfectLink::socket_receive() {

    //TODO
    std::cout << "begin socket listenning" << std::endl;


    while(true) {
        sockaddr_in src_addr;
        memset(&src_addr, 0, sizeof(src_addr));

        socklen_t len;
        ssize_t tot_len;
        char buffer[MAXLINE];

        // receive new message
        if((tot_len = recvfrom(sockfd, const_cast<char *> (buffer), MAXLINE, MSG_WAITALL,
            reinterpret_cast<sockaddr*>(&src_addr), &len)) < 0) {
            std::cerr << "receive msg or ack failed" << std::flush;
            exit(EXIT_FAILURE);
        }
        buffer[tot_len] = '\0';

        std::cout << src_addr.sin_addr.s_addr << " "<< src_addr.sin_port << std::endl;

        int host_id = get_id_from_sockaddr(&src_addr);
        int seq_nr;
        std::memcpy(&seq_nr, buffer+1, sizeof(int));

        std::cout << "receive from " << host_id << " seq nr = " << seq_nr;

        // check if it is an ack
        if(static_cast<bool>(buffer[0])) {
            ACK ack;
            ack.host_id = host_id;
            ack.seq_nr = seq_nr;

            // TODO
            std::cout << " is an ack " << std::endl;

            handle_ack(ack);

        } else {
            // TODO
            std::cout << " is a message... ";

            receive(host_id, seq_nr, &buffer[MSG_START]);
        }
    }
}

void PerfectLink::socket_send(sockaddr_in* addr, int seq_nr, const char* msg) {
    //TODO
    std::cout << "in socket send " << msg << std::endl;
    std::cout << "dest is " << addr->sin_addr.s_addr << " " << addr->sin_port << std::endl;
    
    sendto(sockfd, msg, MAXLINE, 0, reinterpret_cast<const sockaddr*>(addr), sizeof(*addr));
}

void PerfectLink::handle_ack(ACK ack) {
    for(auto it = lacking_acks.begin(); it != lacking_acks.end();) {
        // remove ack from the list of lacking ACKs
        if(it->host_id == ack.host_id and it->seq_nr == ack.seq_nr) {
            // TODO remove
            std::cout << "ack found, remove!" << std::endl;

            it = lacking_acks.erase(it);

        } else {
            ++it;
        }
    }   
}

void PerfectLink::setup_sockaddrs(std::vector<Parser::Host>& hosts) {
    sockaddrs.resize(hosts.size());

    for (auto &host : hosts) {
        sockaddr_in sockaddr;
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_addr.s_addr = host.ip;
        sockaddr.sin_port = host.port;

        sockaddrs[host.id-1] = sockaddr;
    }
}

sockaddr_in* PerfectLink::get_sockaddr_from_id(int id) {
    if(id < static_cast<int>(sockaddrs.size())) {
        return &(sockaddrs[id-1]);
    }
    return nullptr;
}

int PerfectLink::get_id_from_sockaddr(sockaddr_in* addr) {
    int len(static_cast<int>(sockaddrs.size()));
    for(int i(0); i < len; i++) {
        sockaddr_in ref = sockaddrs[i];
        if(ref.sin_port == addr->sin_port && ref.sin_addr.s_addr == addr->sin_addr.s_addr) {
            return i+1;
        }
    }
    return -1;
}