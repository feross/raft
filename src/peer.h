#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>
#include "stream_parser.h"

class Peer {
  public:
    Peer(unsigned short listening_port, std::string destination_ip_address, unsigned short destination_port, std::function<void(Peer*, char*, int)> callback);
    void SendMessage(const char* message, int message_len);
    // ~Peer();

    //unclear how to recieve message, should maybe interrupt main line of execution... or at least, the main program needs to become aware of what this message was
    //for now just have a thread that prints incoming, created in the constructor

  private:
    int AcceptConnection(const char* ip_addr, int port_num);
    int InitiateConnection(const char* ip_addr, int port_num);
    void ListenForInboundMessages();
    void ListenForClose();
    void ReceiveMessages(int socket);

    int send_socket;
    // int receive_socket;
    //in theory could unify, but complicates implementation & creates opportunity for more race conditions, so for now avoid.
    // May go back and change implementation in future, which shouldn't require modifications to the interface

    int my_port;
    int dest_port;
    std::string dest_ip_addr;
    std::thread in_listener;
    std::thread out_listener;
    bool connection_reset;

    StreamParser *stream_parser;
};
