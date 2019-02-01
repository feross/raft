#include "peer.h"
#define RECEIVE_BUFFER_SIZE 100000

static void ErrorCheckSysCall(int success, const char* unique_error_message) {
    if (success == -1) {
        LOG(WARN) << "Error: " << unique_error_message << ", " <<
            strerror(errno) << "(" << errno << ")";
    }
}

Peer::Peer(unsigned short listening_port, std::string destination_ip_address,
        unsigned short destination_port,
        std::function<void(Peer*, char*, int)> message_received_callback) {

    assert(listening_port != destination_port);

    my_port = listening_port;
    dest_port = destination_port;
    dest_ip_addr = destination_ip_address;
    connection_reset = false;
    running = true;
    send_socket = -1;
    receive_socket = -1;

    RegisterReceiveListener();
    stream_parser = new StreamParser([this,
        message_received_callback](char* raw_message, int raw_message_len) {
        message_received_callback(this, raw_message, raw_message_len);
    });
}

Peer::~Peer() {
    running = false;
    if (receive_socket != -1) {
        ErrorCheckSysCall(shutdown(receive_socket, SHUT_RDWR), "shudown");
        //not safe to close until called this
        //see: https://stackoverflow.com/a/2489066/6227019

        //closing the socket is handled by the thread now
        in_listener.join();
    }
    if (connection_reset == true) { //may not have opened a connection to send
        ErrorCheckSysCall(shutdown(send_socket, SHUT_RDWR), "shutdown");
        //closing the socket is handled by the thread now
        out_listener.join();
    }
}

void Peer::SendMessage(const char* message, int message_len) {
    if (send_socket == -1) {
    //TODO: maybe this should be done in a thread to go faster, particular
    //when attempting reconnection, though that should be rare
        LOG(DEBUG) << "Attempted reconnection";
        InitiateConnection(dest_ip_addr.c_str(), dest_port);
    }
    if (send_socket > 0) {
        LOG(DEBUG) << "Sending over socket: " << send_socket;
        auto [formatted_message, formatted_message_len] =
        stream_parser->CreateMessageToSend(message, message_len);
        ErrorCheckSysCall(send(send_socket, formatted_message,
            formatted_message_len, 0), "send");
        delete(formatted_message);
    }
}

void Peer::RegisterReceiveListener() {
    LOG(DEBUG) << "creating a new inbound listening thread";
    in_listener = std::thread([this] () {
        while(running) {
            AcceptConnection(dest_ip_addr.c_str(), my_port);
            LOG(DEBUG) << "receive socket: " << receive_socket;
            ListenOnSocket(receive_socket);
            if (receive_socket != -1) {
                ErrorCheckSysCall(close(receive_socket),"close receive_socket");
                receive_socket = -1;
            }
            stream_parser->ResetIncomingMessage();
            //dump anything we haven't used from this previous connection
        }
    });
}

void Peer::RegisterCloseListener() {
    if (connection_reset) {
        out_listener.join();
        connection_reset = false;
        LOG(DEBUG) << "Joined old listening-for-close-on-outbound thread";
    }
    LOG(DEBUG) << "Creating a new outbound-close listening thread";
    out_listener = std::thread([this] () {
        ListenOnSocket(send_socket);
        connection_reset = true;
        close(send_socket);
        send_socket = -1;
        LOG(DEBUG) << "Outbound Connection Closed";
    });
}

void Peer::ListenOnSocket(int socket) {
    char buffer[RECEIVE_BUFFER_SIZE];
    while(socket > 0 && running) {
        int len = recv(socket, buffer, RECEIVE_BUFFER_SIZE, 0);
        if (len == -1) {
            LOG(DEBUG) << "recv: " << strerror(errno) << "(" << errno << ")";
            break;
        } else if (len == 0) {
            LOG(DEBUG) << "Peer Disconnected";
            break;
        }
        LOG(DEBUG) << "Received " << len << " bytes.";
        stream_parser->HandleRecievedChunk(buffer, len);
    }
}

void Peer::AcceptConnection(const char* ip_addr, unsigned short listening_port) {
    struct sockaddr_in dest; /* socket info about the machine connecting to us */
    struct sockaddr_in serv; /* socket info about our server */
    int mysocket;            /* socket used to listen for incoming connections */
    socklen_t socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));             /* zero the struct */
    serv.sin_family = AF_INET;                  /* set connection type TCP/IP */
    serv.sin_addr.s_addr = htonl(INADDR_ANY); /* accept on any interface */
    serv.sin_port = htons(listening_port);      /* set the server port number */

    mysocket = socket(AF_INET, SOCK_STREAM, 0);
    ErrorCheckSysCall(mysocket, "socket");
    LOG(DEBUG) << "listen_port:" << listening_port << ",mysocket: " << mysocket;

    int val = 1; //required for setsockopt
    ErrorCheckSysCall(setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, &val,
        sizeof(int)), "setsockopt");

    /* bind serv information to mysocket */
    ErrorCheckSysCall(bind(mysocket, (struct sockaddr *)&serv,
        sizeof(struct sockaddr)), "bind");

    /* start listening, allowing a queue of up to 1 pending connection */
    ErrorCheckSysCall(listen(mysocket, 1), "listen");

    // TODO: could just have one server here, and
    // use the sockets returned by this to form peers.
    // However, this would require peers to identify each other, and
    // the class doing this handoff to be simultaneously aware of multiple
    // peers and sockets/networking details.
    receive_socket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
    ErrorCheckSysCall(receive_socket, "accept");

    if (dest.sin_addr.s_addr != inet_addr(ip_addr)) {
        LOG(WARN) << "Connection from unspecified IP, closing connection";
        close(receive_socket);
        receive_socket = -1;
    }

    ErrorCheckSysCall(close(mysocket), "close mysocket");
}

void Peer::InitiateConnection(const char* ip_addr,
    unsigned short destination_port) {
    struct sockaddr_in dest;
    send_socket = socket(AF_INET, SOCK_STREAM, 0);
    ErrorCheckSysCall(send_socket, "send_socket");

    memset(&dest, 0, sizeof(dest));               /* zero the struct */
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(ip_addr);    /* set destination IP num */
    dest.sin_port = htons(destination_port);      /* set destination port num */

    int success = connect(send_socket, (struct sockaddr *)&dest,
        sizeof(struct sockaddr_in));
    if (success == 0) {
        RegisterCloseListener();
    } else {
        send_socket = -1;
        if (success == -1) {
            LOG(DEBUG) << "connect error: " << strerror(errno)
            << " (" << errno << ")";
        }
    }
}
