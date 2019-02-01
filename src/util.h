#pragma once

#include <algorithm>
#include <google/protobuf/message.h>

using namespace std;

typedef ::google::protobuf::Message Message;

class Util {
    public:
        static const string ProtoDebugString(Message& message);
        static const vector<string> StringSplit(const string &str, string delim);
        static string PadRight(string const& str, size_t s);
        static string PadLeft(string const& str, size_t s);
};
