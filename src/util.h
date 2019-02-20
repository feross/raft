/**
 * Utility functions that don't quite fit anywhere else.
 */

#pragma once

#include <algorithm>
#include <google/protobuf/message.h>

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
};
