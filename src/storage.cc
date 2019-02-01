#include "storage.h"

Storage::Storage(string storage_path) : storage_path(storage_path) {
    fstream input(storage_path, ios::in | ios::binary);
    if (!storage_message.ParseFromIstream(&input)) {
        LOG(DEBUG) << "Failed to load storage file; using empty storage";
        Init();
        Save();
    }
    LOG(INFO) << "Initial Storage: " << Util::ProtoDebugString(storage_message);
}

void Storage::Reset() {
    if (remove(storage_path.c_str()) != 0) {
        throw StorageFileException();
    }
}

int Storage::current_term() const {
    return storage_message.current_term();
}

void Storage::set_current_term(int value) {
    storage_message.set_current_term(value);
    Save();
}

const string& Storage::voted_for() const {
    return storage_message.voted_for();
}

void Storage::set_voted_for(const string& value) {
    storage_message.set_voted_for(value);
    Save();
}

vector<string> Storage::log () const {
    google::protobuf::RepeatedPtrField repeatedPtr = storage_message.log();
    return vector<string>(repeatedPtr.begin(), repeatedPtr.end());
}

const string& Storage::log (int index) const {
    return storage_message.log(index);
}

int Storage::log_size() const {
    return storage_message.log_size();
}

void Storage::set_log(int index, const string& value) {
    storage_message.set_log(index, value);
    Save();
}

void Storage::add_log(const string& value) {
    storage_message.add_log(value);
    Save();
}

void Storage::Init() {
    storage_message.set_current_term(0);
    storage_message.set_voted_for("");
    storage_message.clear_log();
}

void Storage::Save() {
    fstream output(storage_path, ios::out | ios::trunc | ios::binary);
    if (!storage_message.SerializeToOstream(&output)) {
        throw StorageFileException();
    }
}
