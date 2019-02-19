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
        std::function<void(Peer*, char*, int)> peer_message_received_callback) {
    assert(listening_port != destination_port);
    my_port = listening_port;
    dest_port = destination_port;
    dest_ip_addr = destination_ip_address;
    connection_reset = false;
    running = true;
    send_socket = -1;
    receive_socket = -1;

    RegisterReceiveListener();

    // TODO moved StreamParser Variables
    partial_number_bytes = 0;
    current_message_length = 0;
    target_message_length = -1;
    message_under_construction = NULL;
    message_received_callback = peer_message_received_callback;
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
    //TODO: maybe this could be done in a thread to go faster, particular
    //when attempting reconnection.  Though that should be rare and that
    //the much-more-common send method below is non-blocking
        LOG(DEBUG) << "Attempted reconnection";
        InitiateConnection(dest_ip_addr.c_str(), dest_port);
    }
    if (send_socket > 0) {
        LOG(DEBUG) << "Sending over socket: " << send_socket;
        //because each peer is sequential, this is fine
        ErrorCheckSysCall(send(send_socket, &message_len,
            sizeof(int), 0), "send int message_len prepend");
        ErrorCheckSysCall(send(send_socket, message,
            message_len, 0), "send message itself");
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
            ResetIncomingMessage();
            //dump anything we haven't used from this previous connection
        }
    });
}

void Peer::RegisterCloseListener() {
    if (connection_reset) { //TODO comment why this is here
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
        HandleRecievedChunk(buffer, len);
    }
}

void Peer::AcceptConnection(const char* ip_addr, unsigned short listening_port) {
    struct sockaddr_in dest; /* socket info about the machine connecting to us */
    struct sockaddr_in serv; /* socket info about our server */
    int mysocket;            /* socket used to listen for incoming connections */
    socklen_t socksize = sizeof(struct sockaddr_in);

    memset(&serv, 0, sizeof(serv));             /* zero the struct */
    serv.sin_family = AF_INET;                  /* set connection type TCP/IP */
    serv.sin_addr.s_addr = htonl(INADDR_ANY);   /* accept on any interface */
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

    // TODO: could just have one server here, and use the sockets returned by
    // this to form peers.
    // However, this would require peers to identify each other using messages,
    // and have an enclosing class doing this handoff from "made connection to
    // someone" to individual peers, and this class would need to be simultaneously
    // aware of multiple peers and sockets/networking details.
    receive_socket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
    ErrorCheckSysCall(receive_socket, "accept");

    if (dest.sin_addr.s_addr != inet_addr(ip_addr)) {
        LOG(WARN) << "Connection from unspecified IP, closing connection";
        close(receive_socket);
        receive_socket = -1;
    }

    ErrorCheckSysCall(close(mysocket), "close mysocket");
}

void Peer::InitiateConnection(const char* ip_addr, //MAYBE TODO: merge into send, & just group via whitespace & inline comments
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





void Peer::ResetIncomingMessage() {
    partial_number_bytes = 0;
    current_message_length = 0;
    target_message_length = -1;
    if (message_under_construction != NULL) delete(message_under_construction);
    message_under_construction = NULL;
}


void Peer::HandleRecievedChunk(char* buffer, int valid_bytes) {
    LOG(DEBUG) << "the buffer, assuming leading int: " << (buffer + sizeof(int));
    // loop necessary, because may have received multiple messages in chunk
    while(valid_bytes > 0) {
        if (target_message_length == -1) {
            //read bytes to determine the next message's length
            int bytes_needed = sizeof(int) - partial_number_bytes;
            int message_len_bytes = bytes_needed;
            if(valid_bytes < bytes_needed) {
                message_len_bytes = valid_bytes;
            }
            memcpy(incomplete_number_buffer + partial_number_bytes,
            buffer, message_len_bytes);
            partial_number_bytes += message_len_bytes;
            valid_bytes -= message_len_bytes;
            if(message_len_bytes == bytes_needed) {
                // complete message_len_number was obtained,
                target_message_length = *(int*)incomplete_number_buffer;
                current_message_length = 0;

                buffer = buffer + bytes_needed;
                //heap allocation is not avoidable if e.g. we receive only part
                // of the full message
                message_under_construction = new char[target_message_length + 1];
                message_under_construction[target_message_length] = '\0';
                //null-terminate only because it's low-cost to do so
            }
        } else {
            // accumulate current message
            int bytes_to_copy = target_message_length;
            if (valid_bytes < target_message_length) bytes_to_copy = valid_bytes;
            memcpy(message_under_construction + current_message_length,
                buffer, bytes_to_copy);
            buffer = buffer + bytes_to_copy;
            current_message_length += bytes_to_copy;
            valid_bytes -= bytes_to_copy;

            LOG(DEBUG) << "current message len: " << current_message_length <<
                ", target: " << target_message_length << ", valid: " <<
                valid_bytes << ", to_copy: " << bytes_to_copy;

            // if accumulated full message, callback & reset internal data
            if(current_message_length == target_message_length) {
                LOG(DEBUG) << "Found full message: " <<
                    message_under_construction << ", buffer: " << buffer;

                message_received_callback(this, message_under_construction,
                    target_message_length);
                message_under_construction = NULL;
                // we are no longer owner of this data, client's job to manage
                ResetIncomingMessage();
            }
        }
    }
}




























