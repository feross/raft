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
// clean this up ^

#include "stream_parser.h"

#define DEBUG true

StreamParser::StreamParser(void callback(char* message, int message_len)) {
    current_message_length = 0;
    target_message_length = -1;
    message_under_construction = NULL;
    message_received_callback = callback;
}

void StreamParser::HandleRecievedChunk(char* buffer, int valid_bytes) {
    if (DEBUG) printf("\n the raw buffer: %s\n\n", buffer + 4);
    while(valid_bytes > 0) {
        if (target_message_length == -1) {
            assert(valid_bytes >= sizeof(int));
            target_message_length = *(int*)buffer; // TODO: assumes that buffer contains the full integer, should really handle the split case though
            current_message_length = 0;

            buffer = buffer + sizeof(int);
            valid_bytes -= sizeof(int);
            message_under_construction = new char[target_message_length + 1];
            message_under_construction[target_message_length] = '\0'; //null-terminate b/c it's low-cost to do so
        }

        int bytes_to_copy = target_message_length;
        if (valid_bytes < target_message_length) bytes_to_copy = valid_bytes;
        memcpy(message_under_construction + current_message_length, buffer, bytes_to_copy);
        buffer = buffer + bytes_to_copy;
        current_message_length += bytes_to_copy;
        if (DEBUG) printf("current message len: %d, target: %d, valid: %d, to_copy: %d\n", current_message_length, target_message_length, valid_bytes, bytes_to_copy);
        valid_bytes -= bytes_to_copy;

        if(current_message_length == target_message_length) {
            if (DEBUG) printf("\nfound full message! : %s, buffer: %s\n\n", message_under_construction, buffer);
            message_received_callback(message_under_construction, target_message_length);
            // printf("complete message received: %s", message_under_construction); //TODO: should use callback
            ResetIncomingMessage();
        }
    }
}

//side-effect: deletes existing accumulation
void StreamParser::ResetIncomingMessage() {
    current_message_length = 0;
    target_message_length = -1;
    if (message_under_construction != NULL) delete(message_under_construction);
    message_under_construction = NULL;
}

char* StreamParser::CreateMessageToSend(const char* raw_message, int message_len) {
    char* send_buffer = new char[message_len + sizeof(int)];
    *(int*)send_buffer = message_len;
    memcpy(send_buffer + sizeof(int), raw_message, message_len);
    printf("raw: %s, created: %s\n", raw_message, send_buffer);
    return send_buffer;   // Should really return a struct, or similar.  Because that would allow stack usage & be more explicit
}