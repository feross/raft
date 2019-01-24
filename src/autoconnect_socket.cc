#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#include <iostream>
#include <string>

#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>
#include <thread>
#include <cassert>

#include "peer.h"

#define MAXRCVLEN 500
#define PORTNUM 2300

 
int main(int argc, char *argv[])
{
    //argument format: my_port, connect_port
    int my_port = std::stoi(argv[1]);
    int connect_port = std::stoi(argv[2]);
    assert(my_port != connect_port); // hack for now, b/c we're on localhost & no other way to distinguish connections

    const char* msg = "wow !    ";

    const char* dest_addr = "127.0.0.1";
    

    Peer* associate = new Peer(my_port, dest_addr, connect_port);

    while (true) {
        // if (my_port < connect_port) {
        //     sleep(2);
        // }
        associate->SendMessage(msg);
        sleep(1);
    }
    delete(associate);

    return EXIT_SUCCESS;
}
