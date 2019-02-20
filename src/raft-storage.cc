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

const string& RaftStorage::voted_for() const {
    return storage_message.voted_for();
}

void RaftStorage::set_vote_and_term(const string& vote, int term) {
    storage_message.set_voted_for(vote);
    storage_message.set_current_term(term);
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
    storage_message.SerializeToString(storage_string);
    if (Util::PersistentFileUpdate(storage_path, storage_string.c_str(),
        storage_string.length()) == false) {
        throw RaftStorageException("Failed to write storage: " + storage_path);
    }
}
