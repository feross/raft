#include "util.h"

const string Util::ProtoDebugString(::google::protobuf::Message& message) {
    string str = message.DebugString();
    // Remove trailing newlinem
    str = str.substr(0, str.size() - 1);
    // Replace newlines with commas for a more compact representation
    replace(str.begin(), str.end(), '\n', ',');
    return  str;
}
