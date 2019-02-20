#include "raft-storage.h"

RaftStorage::RaftStorage(string storage_path) : storage_path(storage_path) {
    fstream input(storage_path, ios::in | ios::binary);
    if (!storage_message.ParseFromIstream(&input)) {
        throw RaftStorageException("Failed to read storage: " + storage_path);
        storage_message.set_current_term(0);
        storage_message.set_voted_for("");
        Save();
    }
    debug("Initial Storage: %s", Util::ProtoDebugString(storage_message).c_str());
}

void RaftStorage::Reset() {
    if (remove(storage_path.c_str()) != 0) {
        throw RaftStorageException("Could not remove storage: " + storage_path);
    }
}

int RaftStorage::current_term() const {
    return storage_message.current_term();
}

void RaftStorage::set_current_term(int value) {
    storage_message.set_current_term(value);
    Save();
}

const string& RaftStorage::voted_for() const {
    return storage_message.voted_for();
}

void RaftStorage::set_voted_for(const string& value) {
    storage_message.set_voted_for(value);
    Save();
}

void RaftStorage::Save() {
    fstream output(storage_path, ios::out | ios::trunc | ios::binary);
    if (!storage_message.SerializeToOstream(&output)) {
        throw RaftStorageException("Failed to write storage: " + storage_path);
    }
}
