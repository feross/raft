#include "raft-storage.h"

RaftStorage::RaftStorage(string storage_path) : storage_path(storage_path) {}

void RaftStorage::Load() {
    fstream input(storage_path, ios::in | ios::binary);
    if (!storage_message.ParseFromIstream(&input)) {
        throw RaftStorageException("Failed to read storage: " + storage_path);
    }
    debug("Loaded storage: %s", Util::ProtoDebugString(storage_message).c_str());
}

void RaftStorage::Reset() {
    storage_message.Clear();
    storage_message.set_current_term(0);
    storage_message.set_voted_for(-1);
    storage_message.set_last_applied(0);
    Save();
}

int RaftStorage::current_term() const {
    return storage_message.current_term();
}

int RaftStorage::voted_for() const {
    return storage_message.voted_for();
}

void RaftStorage::set_term_and_voted(int current_term, int voted_for) {
    storage_message.set_current_term(current_term);
    storage_message.set_voted_for(voted_for);
    Save();
}

int RaftStorage::last_applied() const {
    return storage_message.last_applied();
}

void RaftStorage::set_last_applied(int value) {
    storage_message.set_last_applied(value);
    Save();
}

void RaftStorage::Save() {
    string storage_string;
    storage_message.SerializeToString(&storage_string);
    if (Util::PersistentFileUpdate(storage_path.c_str(), storage_string.c_str(),
        storage_string.length()) == false) {
        throw RaftStorageException("Failed to write storage: " + storage_path);
    }
}
