#include "approx_tcp.hpp"
#include "network_unit.hpp"

#include <limits>

ApproxTCP::ApproxTCP(const sockaddr_in *host_addr, NetworkUnit *upper_layer)
    : upper_layer(upper_layer)
{
    if (host_addr == nullptr)
    {
        std::cerr << "host socket adress cannot be null" << std::endl;
        exit(EXIT_FAILURE);
    }
    addr = host_addr;

    sockfd = create_bind_socket();
}

void ApproxTCP::start()
{
    std::thread recv_t(&ApproxTCP::socket_polling, this);
    std::thread send_t(&ApproxTCP::socket_pushing, this);

    recv_t.detach();
    send_t.detach();
}

void ApproxTCP::socket_polling()
{
    sockaddr_in *src_addr = new sockaddr_in;
    memset(src_addr, 0, sizeof(&src_addr));
    socklen_t len = sizeof(&src_addr);

    ssize_t tot_len;
    char buffer[MAXLINE];

    while (true)
    {
        // receive new message
        if ((tot_len = recvfrom(sockfd, const_cast<char *>(buffer), MAXLINE, MSG_WAITALL,
                                reinterpret_cast<sockaddr *>(src_addr), &len)) < 0)
        {
            std::cerr << "receive msg or ack failed" << std::flush;
            exit(EXIT_FAILURE);
        }
        if (tot_len > MAXLINE)
        {
            std::cerr << "Received message is too long" << std::endl;
            exit(EXIT_FAILURE);
        }
        buffer[tot_len] = '\0';

        // We want the ptr to the address stored in the upper_layer unit
        const sockaddr_in *known_src_addr = upper_layer->sockaddr_local(*src_addr);
        if (known_src_addr == nullptr)
        {
            std::cerr << "receive msg from unkown source" << std::flush;
            exit(EXIT_FAILURE);
        }

        // build message
        bool is_ack;
        Message message;
        extract_from_udp_packet(is_ack, message, buffer);

        if (is_ack)
        {
            ACK ack;
            ack.message.seq_nr = message.seq_nr;
            ack.message.msg = nullptr;
            handle_ack(ack);
        }
        else
        {
            socket_receive(known_src_addr, message);
        }

        // free the message
        free(message.msg);
    }
}

void ApproxTCP::socket_pushing()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        for (auto &ack : lacking_acks)
        {
            char *msg = ack.message.msg;
            const sockaddr_in *addr = ack.addr;
            sendto(sockfd, msg, MAXLINE, 0, reinterpret_cast<const sockaddr *>(addr), sizeof(*addr));
        }
    }
}

void ApproxTCP::socket_send(const sockaddr_in *dest, const Message &message)
{
    char *buffer = static_cast<char *>(malloc(MAXLINE));
    build_udp_packet(false, message, buffer);

    // build ACK
    ACK *ack = new ACK;
    ack->addr = dest;
    ack->message.seq_nr = message.seq_nr;
    ack->message.msg = buffer;

    // ack already in lacking_acks
    for (auto &a : lacking_acks)
    {
        if (a.isEqual(*ack))
            return;
    }

    sendto(sockfd, buffer, MAXLINE, 0, reinterpret_cast<const sockaddr *>(dest), sizeof(*dest));

    // add new ack in lacking_acks
    lacking_acks.push_back(*ack);
}

void ApproxTCP::socket_receive(const sockaddr_in *src, const Message &message)
{
    char buffer[MAXLINE];
    build_udp_packet(true, message, buffer);
    sendto(sockfd, buffer, MAXLINE, 0, reinterpret_cast<const sockaddr *>(src), sizeof(*src));

    int id = upper_layer->id_from_sockaddr(src);
    upper_layer->receive(id, message);
}

void ApproxTCP::handle_ack(ACK &ack)
{
    // simply remove it form lacking_acks
    auto it = lacking_acks.begin();
    while (it != lacking_acks.end())
    {
        if (it->isEqual(ack))
        {
            free(it->message.msg);
            it = lacking_acks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

int ApproxTCP::create_bind_socket()
{
    // Creating socket file descriptor
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cerr << "socket creation failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Bind the socket with the server address
    if (bind(sockfd, reinterpret_cast<const sockaddr *>(addr), sizeof(*addr)) < 0)
    {
        std::cerr << "binding failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void ApproxTCP::build_udp_packet(bool is_ack, const Message &message, char *buffer)
{
    // set ack
    buffer[0] = is_ack ? static_cast<char>(1) : static_cast<char>(0);

    // set seq_nr
    int seq_nr = message.seq_nr;
    memcpy(buffer + 1, reinterpret_cast<char *>(&seq_nr), sizeof(int));

    // set message
    strncpy(buffer + MSG_START, message.msg, MSG_SIZE);
    buffer[MAXLINE - 1] = '\0';
}

void ApproxTCP::extract_from_udp_packet(bool &is_ack, Message &message, const char *udp_packet)
{
    is_ack = static_cast<bool>(udp_packet[0]);
    std::memcpy(&(message.seq_nr), udp_packet + 1, sizeof(int));

    char *msg = static_cast<char *>(malloc(MSG_SIZE + 1));
    std::strncpy(msg, udp_packet + MSG_START, MSG_SIZE);
    msg[MSG_SIZE] = '\0';
    message.msg = msg;
}
