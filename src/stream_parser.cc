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
#include <tuple>
// TODO: clean this up ^

#include "stream_parser.h"

#define DEBUG false

/**
 * General implementation idea - we receive chunks of data that any number of bytes formatted in the following way:
 *      Format: (prepended integer length 1) + message 1 + (prepended integer length 2) + message 2 + ...
 *
 * We need to handle all possible splits of this data, including partial prepended numbers split across received chunks, multiple messages in a single chunk, etc.
 * To do this, we attempt to accumulate a message, and save any partial state between calls to HandleReceivedChunk
 * Sending is simpler: we just create a buffer, prepend a number, and return (heap-allocated)
 */

StreamParser::StreamParser(std::function<void(char*, int)> callback) {
    partial_number_bytes = 0;
    current_message_length = 0;
    target_message_length = -1;
    message_under_construction = NULL;
    message_received_callback = callback;
}

StreamParser::~StreamParser() {
    
}

void StreamParser::HandleRecievedChunk(char* buffer, int valid_bytes) {
    if (DEBUG) printf("\n the buffer, assuming leading int: %s\n\n", buffer + sizeof(int));
    while(valid_bytes > 0) {
        if (target_message_length == -1) {
            int bytes_needed = sizeof(int) - partial_number_bytes;
            if(valid_bytes < bytes_needed) {
                //partial message_len_number, must save & wait for more from stream
                memcpy(incomplete_number_buffer + partial_number_bytes, buffer, valid_bytes);
                partial_number_bytes += valid_bytes;
                valid_bytes -= valid_bytes;
                break;
            } else {
                //complete message_len_number obtainable, get & create buffer in which to place message
                memcpy(incomplete_number_buffer + partial_number_bytes, buffer, bytes_needed);
                partial_number_bytes += bytes_needed;
                valid_bytes -= bytes_needed;

                target_message_length = *(int*)incomplete_number_buffer;
                current_message_length = 0;

                buffer = buffer + bytes_needed;
                message_under_construction = new char[target_message_length + 1];
                message_under_construction[target_message_length] = '\0'; //null-terminate because it's low-cost to do so
            }
        } else {
            // accumulate current message
            int bytes_to_copy = target_message_length;
            if (valid_bytes < target_message_length) bytes_to_copy = valid_bytes;
            memcpy(message_under_construction + current_message_length, buffer, bytes_to_copy);
            buffer = buffer + bytes_to_copy;
            current_message_length += bytes_to_copy;
            if (DEBUG) printf("current message len: %d, target: %d, valid: %d, to_copy: %d\n", current_message_length, target_message_length, valid_bytes, bytes_to_copy);
            valid_bytes -= bytes_to_copy;

            // if accumulated full message, callback & reset internal data
            if(current_message_length == target_message_length) {
                if (DEBUG) printf("\nfound full message! : %s, buffer: %s\n\n", message_under_construction, buffer);
                message_received_callback(message_under_construction, target_message_length);
                message_under_construction = NULL; // we are no longer owner of this data, client's job to manage (e.g. like strdup)
                // printf("complete message received: %s", message_under_construction); //TODO: should use callback
                ResetIncomingMessage();
            }
        }
    }
}

void StreamParser::ResetIncomingMessage() {
    partial_number_bytes = 0;
    current_message_length = 0;
    target_message_length = -1;
    if (message_under_construction != NULL) delete(message_under_construction);
    message_under_construction = NULL;
}

std::tuple<char*, int> StreamParser::CreateMessageToSend(const char* raw_message, int message_len) {
    char* send_buffer = new char[message_len + sizeof(int)];
    *(int*)send_buffer = message_len;
    memcpy(send_buffer + sizeof(int), raw_message, message_len);
    if (DEBUG) printf("raw: %s, created: %s\n", raw_message, send_buffer);
    std::tuple<char*, int> formatted_message_info(send_buffer, message_len + sizeof(int));
    return formatted_message_info;   // Should really return a struct, or similar.  Because that would allow stack usage & be more explicit
}
