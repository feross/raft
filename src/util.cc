#include "util.h"

const string Util::ProtoDebugString(::google::protobuf::Message& message) {
    string str = message.DebugString();
    // Remove trailing newline
    str = str.substr(0, str.size() - 1);
    // Replace newlines with commas for a more compact representation
    replace(str.begin(), str.end(), '\n', ' ');
    return  str;
}

const vector<string> Util::StringSplit(const string &str, string delim) {
    vector<string> result;

    size_t last = 0;
    size_t next = 0;
    for (; (next = str.find(delim, last)) != string::npos; last = next + 1) {
        result.push_back(str.substr(last, next - last));
    }
    result.push_back(str.substr(last));

    return result;
}

string Util::PadRight(string const& str, size_t size) {
    if (str.size() < size) {
        return str + string(size - str.size(), ' ');
    } else {
        return str;
    }
}

string Util::PadLeft(string const& str, size_t size) {
    if (str.size() < size) {
        return string(size - str.size(), ' ') + str;
    } else {
        return str;
    }
}
