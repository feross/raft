#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>

class Peer {
  public:
    Peer(unsigned short listening_port, const char* destination_ip_address, unsigned short destination_port, void f(char* message));
    void SendMessage(const char* message);
    // ~Peer();

    //unclear how to recieve message, should maybe interrupt main line of execution... or at least, the main program needs to become aware of what this message was
    //for now just have a thread that prints incoming, created in the constructor

  private:
    int AcceptConnection(const char* ip_addr, int port_num);
    int InitiateConnection(const char* ip_addr, int port_num);
    void ListenForInboundMessages();
    void ListenForClose();
    void ReceiveMessages(int socket);

    std::mutex modify_socket_lock;
    int send_socket;
    // int receive_socket;
    //in theory could unify, but complicates implementation & creates opportunity for more race conditions, so for now avoid.
    // May go back and change implementation in future, which shouldn't require modifications to the interface

    int my_port;
    int dest_port;
    const char* dest_ip_addr;
    std::thread in_listener;
    std::thread out_listener;
    bool connection_reset;
    void message_received_callback(char* message);


    //perhaps turn into "stream parser" class, or something
    int current_message_length;
    int target_message_length;
    // char incomplete_number[4] - case to handle where the integer delimiting message lengths was split... ignore for now
    char* message_under_construction; //heap allocated buffer to fill into message to return to client
    void HandleRecievedChunk(char* buffer, int valid_bytes); //accumulates from stream & parses into individual messages
    void CreateMessageToSend(char* raw_message); //prepends with an integer to indicate length and sends
};