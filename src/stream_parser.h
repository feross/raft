#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <atomic>
#include <functional>


#pragma once


class StreamParser {
  public:
    StreamParser(void callback(char* message, int message_len));
    void HandleRecievedChunk(char* buffer, int valid_bytes); //accumulates from stream & parses into individual messages
    char* CreateMessageToSend(const char* raw_message, int message_len); //prepends with an integer to indicate length and sends
    void ResetIncomingMessage();


  private:
    std::function<void(char*, int)> message_received_callback;
    int current_message_length;
    int target_message_length;
    // char incomplete_number[4] - case to handle where the integer delimiting message lengths was split... ignore for now
    char* message_under_construction; //heap allocated buffer to fill into message to return to client
};