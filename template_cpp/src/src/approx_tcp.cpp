#include "approx_tcp.hpp"
#include "network_unit.hpp"

#include <climits>
#include <cstring>

ApproxTCP::ApproxTCP(const sockaddr_in *host_addr, NetworkUnit *upper_layer)
    : upper_layer(upper_layer), tcp_seq_nr(0)
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
        message.msg = static_cast<char *>(malloc(MSG_SIZE_TCP + 1));
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

void ApproxTCP::socket_send(const sockaddr_in *dest, const Message &tcp_msg)
{
    char *buffer = static_cast<char *>(malloc(MAXLINE));
    build_udp_packet(false, tcp_msg, buffer);

    // build ACK
    ACK *ack = new ACK;
    ack->addr = dest;
    ack->message.seq_nr = tcp_msg.seq_nr;
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

void ApproxTCP::socket_receive(const sockaddr_in *src, const Message &tcp_msg)
{
    char buffer[MAXLINE];
    build_udp_packet(true, tcp_msg, buffer);
    sendto(sockfd, buffer, MAXLINE, 0, reinterpret_cast<const sockaddr *>(src), sizeof(*src));

    int id = upper_layer->id_from_sockaddr(src);
    upper_layer->receive(id, tcp_msg);
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

void ApproxTCP::build_udp_packet(bool is_ack, const Message &tcp_msg, char *buffer)
{
    // set ack
    buffer[0] = is_ack ? static_cast<char>(1) : static_cast<char>(0);

    // set src_id
    char src_id = static_cast<char>(tcp_msg.src_id);
    buffer[ACK_SIZE] = src_id;

    // set seq_nr
    int seq_nr = tcp_msg.seq_nr;
    memcpy(buffer + ACK_SIZE + SRC_ID_SIZE, reinterpret_cast<char *>(&seq_nr), SEQ_SIZE);

    // set message
    strncpy(buffer + MSG_START_TCP, tcp_msg.msg, MSG_SIZE_TCP);
    buffer[MAXLINE - 1] = '\0';
}

void ApproxTCP::extract_from_udp_packet(bool &is_ack, Message &tcp_msg, const char *udp_packet)
{
    // extract ack
    is_ack = static_cast<bool>(udp_packet[0]);

    // extract src_id
    int src_id = static_cast<int>(udp_packet[ACK_SIZE]);
    if (src_id == CHAR_MIN)
    {
        src_id = CHAR_MAX + 1;
    }
    tcp_msg.src_id = src_id;

    // extract seq_nr
    memcpy(&(tcp_msg.seq_nr), udp_packet + ACK_SIZE + SRC_ID_SIZE, SEQ_SIZE);

    // extract msg
    char *msg = static_cast<char *>(malloc(MSG_SIZE_TCP + 1));
    strncpy(msg, udp_packet + MSG_START_TCP, MSG_SIZE_TCP);
    msg[MSG_SIZE_TCP] = '\0';
    tcp_msg.msg = msg;

    if (udp_packet[MSG_SIZE_TCP + SRC_ID_SIZE] == 0 and udp_packet[MSG_SIZE_TCP + SRC_ID_SIZE + 1] == 0 and udp_packet[MSG_SIZE_TCP + SRC_ID_SIZE + 2] == 0 and udp_packet[MSG_SIZE_TCP + SRC_ID_SIZE + 3] == 0)
    {
        std::cerr << " dddddec error" << std::endl;
        exit(EXIT_FAILURE);
    }
}
