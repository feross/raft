#ifndef _RAFT_UTIL_H_
#define _RAFT_UTIL_H_

#include <algorithm>
#include <google/protobuf/message.h>

using namespace std;

class Util {
    public:
        static const string ProtoDebugString(::google::protobuf::Message& message);
};

#endif
