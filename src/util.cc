#include "util.h"

const string Util::ProtoDebugString(::google::protobuf::Message& message) {
    string str = message.DebugString();
    // Remove trailing newline
    str = str.substr(0, str.size() - 1);
    // Replace newlines with commas for a more compact representation
    replace(str.begin(), str.end(), '\n', ' ');
    return str;
}

const vector<string> Util::StringSplit(const string &str, string delim) {
    vector<string> result;

    size_t last_seen = 0;
    size_t next = 0;
    for (; (next = str.find(delim, last_seen)) != string::npos; last_seen = next + 1) {
        result.push_back(str.substr(last_seen, next - last_seen));
    }
    result.push_back(str.substr(last_seen));

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

// String trim implementation inspired by:
// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
void Util::LeftTrim(string &str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
void Util::RightTrim(string &str) {
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), str.end());
}

// trim from both ends (in place)
void Util::Trim(std::string &str) {
    LeftTrim(str);
    RightTrim(str);
}


bool Util::SyscallErrorInfo(bool syscall_success, const char *error_message_prefix) {
  if (!syscall_success) {
    warn("Error: %s, %s (%d)", error_message_prefix, strerror(errno), errno);
  }
  return syscall_success;
}


bool Util::PersistentFileUpdate(const char * filename, const void * new_contents, int new_contents_len) {
  // assume old file is safe
  std::string tmp_filename = "tmp_" + std::string(filename);
  debug("tmp name: %s", tmp_filename.c_str());
  FILE * tmp_file = fopen( tmp_filename.c_str() , "wb" );
  bool opened = SyscallErrorInfo(tmp_file != NULL,
    "fopen of temp file as 'wb' failed"); //TODO: include filename
  bool written = SyscallErrorInfo(new_contents_len == fwrite(new_contents, 1, new_contents_len, tmp_file),
    "fwrite of new_contents to tmp_file failed"); //TODO include more info
  bool flushed = SyscallErrorInfo(0 == fflush(tmp_file),
    "Error: fflush failed to push all writes to disk, ");
  bool closed = SyscallErrorInfo(0 == fclose(tmp_file), "fclose of tmp_file failed"); //TODO include filename
  if (opened && written && closed && flushed) {
    return SyscallErrorInfo(0 == rename(tmp_filename.c_str(), filename),
    "rename of tmp_filename to filename failed"); //TODO: include more info
  }
  warn("File Update Failed %s, %s, %d", filename, new_contents, new_contents_len);
  return false;
}
