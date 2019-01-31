#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>
#include <tuple>

#pragma once

class StreamParser {
  public:
    /**
     * Tool for parsing objects to be passed into a stream, or pulled out of a stream of bytes.
     * 
     * Must be used by both ends of the stream to work properly, due to encoding scheme
     *
     * @param callback - callback function to be called whenever we receive a message
     *                   callback arguments:
                            char* - pointer to heap-allocated received message data, which the client is responsible for freeing
                            int - size of message data
     */
    StreamParser(std::function<void(char*, int)> callback);

    /**
     * Given a chunk of data from the stream of any size:
     *  - parses out any completed messages & calls message_received_callback that was passed in the constructor
     *  - accumulates any partial messages to be completed by future received chunks.
     *
     * Must be called on bytes coming from stream in the order they come through.
     *
     * @param buffer - buffer containing received bytes from the stream
     * @param valid_bytes - number of valid bytes received from the stream, in this buffer
     */
    void HandleRecievedChunk(char* buffer, int valid_bytes);

    //Heap allocates a wrapped set of bytes to send to other machines
    /**
     * Given a message to send and its size, generates the buffer to be sent to the other end of the stream.
     * This buffer is heap-allocated & returned to the client, who is responsible for freeing it.
     * 
     * @param raw_message - buffer containing data we wish to send
     * @param message_len - length (in bytes) of the message in the raw_message buffer
     * 
     * @return - a tuple containing:
     *                  char* - a heap-allocated, formatted buffer to send, which the client is responsible for freeing
     *                  int - the size of the data in that buffer
     */
    std::tuple<char*, int> CreateMessageToSend(const char* raw_message, int message_len);

    /**
     * Resets any partially accumulated message from the stream, which is thrown out entirely.
     */
    void ResetIncomingMessage();

  private:
    std::function<void(char*, int)> message_received_callback;
    int current_message_length;
    int target_message_length;
    char* message_under_construction; //heap allocated buffer to fill into message to return to client

    int partial_number_bytes;
    char incomplete_number_buffer[sizeof(int)];
};
