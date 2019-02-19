#include "storage.h"

Storage::Storage(string storage_path) : storage_path(storage_path) {
    fstream input(storage_path, ios::in | ios::binary);
    if (!storage_message.ParseFromIstream(&input)) {
        throw StorageException("Failed to read storage: " + storage_path);
        storage_message.set_current_term(0);
        storage_message.set_voted_for("");
        Save();
    }
    LOG(DEBUG) << "Initial Storage: " << Util::ProtoDebugString(storage_message);
}

void Storage::Reset() {
    if (remove(storage_path.c_str()) != 0) {
        throw StorageException("Could not remove storage: " + storage_path);
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

void Storage::Save() {
    fstream output(storage_path, ios::out | ios::trunc | ios::binary);
    if (!storage_message.SerializeToOstream(&output)) {
        throw StorageException("Failed to write storage: " + storage_path);
    }
}
