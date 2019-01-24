#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>

class Peer {
  public:
    Peer(unsigned short source_port, const char* destination_ip_address, unsigned short destination_port);
    void SendMessage(const char* message);
    // ~Peer();

    //unclear how to recieve message, should maybe interrupt main line of execution... or at least, the main program needs to become aware of what this message was
    //for now just have a thread that prints incoming, created in the constructor

  private:
    int AcceptConnection(const char* ip_addr, int port_num);
    int AttemptConnection(const char* ip_addr, int port_num);
    void StartListener();
    int consocket;
    int my_port;
    int dest_port;
    const char* dest_ip_addr;
    std::thread listener;
};