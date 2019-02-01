#pragma once

#include "log.h"

class StreamParser {
    public:
        /**
         * Tool for parsing objects to be passed into a stream,
         * or pulled out of a stream of bytes.
         *
         * Must be used by both ends of the stream, due to encoding scheme
         *
         * @param callback - callback function for whenever we receive a message
         *      callback arguments:
         *          char* - pointer to heap-allocated received message
         *              data, which the client is responsible for freeing
         *          int - size of message data
         */
        StreamParser(std::function<void(char*, int)> callback);

        /**
         * Destroy the StreamParser & clean up all resources
         */
        ~StreamParser();

        /**
         * Given a chunk of data from the stream of any size:
         *  - parses out any completed messages & calls message_received_callback
         *      that was passed in the constructor
         *  - accumulates any partial messages to be completed by future chunks.
         *
         * Must be called on bytes coming from stream in order.
         *
         * @param buffer - buffer containing received bytes from the stream
         * @param valid_bytes - number of valid bytes in this buffer from stream
         */
        void HandleRecievedChunk(char* buffer, int valid_bytes);

        /**
         * Given a message to send and its size, generates the buffer to be sent
         * to the other end of the stream.  This buffer is heap-allocated &
         * returned to the client, who is responsible for freeing it.
         *
         * @param raw_message - buffer containing data we wish to send
         * @param message_len - length (in bytes) of the raw_message buffer
         *
         * @return - a tuple containing:
         *                  char* - a heap-allocated, formatted buffer to send,
                                which the client is responsible for freeing
         *                  int - the size of the data in that buffer
         */
        std::tuple<char*, int> CreateMessageToSend(const char* raw_message,
            int message_len);

        /**
         * Resets/ throws out any partially accumulated message from the stream
         */
        void ResetIncomingMessage();

    private:
        /**
         * Callback invoked every time we have accumulated a "complete" message
         * as defined by "was sent as CreateMessageToSend blob on other end of
         * stream".
         *
         * Callback arguments:
         *      - char* : the blob of bytes received
         *      - int : the length of the blob of bytes received
         */
        std::function<void(char*, int)> message_received_callback;
        int current_message_length;
        int target_message_length;
        char* message_under_construction;

        /**
         * Because the stream of bytes may be cut even along within the middle
         * of the 4 bytes identifying the length of the subsequent message blob,
         * we may need to accumulate a partial integer.  We use this number &
         * buffer to accumulate bytes until the full number is complete.
         */
        int partial_number_bytes;
        char incomplete_number_buffer[sizeof(int)];
};
