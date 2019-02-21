/**
 * Utility functions that don't quite fit anywhere else.
 */

#pragma once

#include <algorithm>
#include <google/protobuf/message.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <string>
#include "log.h"


using namespace std;

typedef ::google::protobuf::Message Message;

class Util {
    public:
        /**
         * Serialize a Protocol Buffer message into anhuman-readable debug
         * string which can be included in logs. This method produces a string
         * that can be printed on a single line, unlike the built in Protocol
         * Buffer debug string which includes numerous newlines.
         *
         * @param  message The Protocol Buffer message to convert to a string
         * @return  String representation of the Protocol Buffer
         */
        static const string ProtoDebugString(Message& message);

        /**
         * Split the given string str into a vector of strings using the given
         * single-character delimiter delim.
         *
         * Example:
         *     StringSplit("12.34.56.78:9000", ":")
         *
         *     Returns a vector with two elements: "12.34.56.78", "9000"
         *
         * @param str The string to split
         * @param delim The delimiter to search for
         * @return Vector of result strings
         */
        static const vector<string> StringSplit(const string &str, string delim);

        /**
         * Add space to the right of the given string str so that it is at least
         * size characters wide.
         *
         * @param  str The string to pad
         * @param  size The size to ensure the string is padded to
         * @return The result string
         */
        static string PadRight(string const& str, size_t size);

        /**
         * Add space to the left of the given string str so that it is at least
         * size characters wide.
         *
         * @param  str The string to pad
         * @param  size The size to ensure the string is padded to
         * @return The result string
         */
        static string PadLeft(string const& str, size_t size);

        /**
         * Trim whitespace from the start of a string, passed by reference.
         * @param str String to "left trim"
         */
        static void LeftTrim(string &str);

        /**
         * Trim whitespace from the end of a string, passed by reference.
         * @param str String to "right trim"
         */
        static void RightTrim(string &str);

        /**
         * Trim whitespace from the start and end of a string, passed by
         * reference.
         * @param str String to trim
         */
        static void Trim(std::string &str);

        /*
         * Log errno & associate message, in addition to the given error message
         * prefix that includes more user-specified information
         *
         * @param syscall_success - syscall completed as expected
         * @param error_message_prefix - additional user-specified info to log
         * @return bool - whether syscall was successful
         */
        static bool SyscallErrorInfo(bool syscall_success, const char *error_message_prefix);

        /*
         * Update filename to have new_contents, overwriting old file contents
         * Will always either completely succeed, or fail.  I.e. filename either
         * has intact old contents, or intact new contents
         *
         * @param filename - file to be updated
         * @param new_contents - pointer to start of new file contents
         * @param new_contents_len - length of new file contents
         * @return bool - whether we successfully updated file to new contents
         *
         * NOTE: creates a whole new file, so new_contents_len should be
         * relatively short.  E.g. don't use for a large log.
         */
        static bool PersistentFileUpdate(const char * filename, const void * new_contents, int new_contents_len);
};
