#ifndef _RAFT_UTIL_H_
#define _RAFT_UTIL_H_

#include <algorithm>
#include <google/protobuf/message.h>

using namespace std;

typedef ::google::protobuf::Message Message;

class Util {
    public:
        static const string ProtoDebugString(Message& message);
        static const vector<string> StringSplit(string str, string delim);
};

#endif
