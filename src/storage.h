#pragma once

#include <fstream>
#include <cstdio>
#include <google/protobuf/repeated_field.h>

#include "log.h"
#include "storage-message.pb.h"
#include "util.h"

using namespace proto;
using namespace std;

const string STORAGE_NAME_SUFFIX = "-storage.dat";

class StorageFileException : public exception {
    const char* what() const noexcept {
        return "Storage file IO error";
    }
};

class Storage {
    public:
        /**
         * Persistent server state for Raft servers, backed by stable storage.
         *
         * This class exposes a friendly interface for getting or setting the
         * persistent server state for a Raft server. All setter methods in this
         * class ensure that data is persisted to stable storage before
         * returning.
         *
         * @param storage_path Path to the storage file to use
         */
        Storage(string storage_path);

        /**
         * Delete the stable storage file.
         */
        void Reset();

        /**
         * Returns the current term. The latest term the server has seen
         * (initialized to 0 on first boot, increases monotonically)
         *
         * @return the current term
         */
        int current_term() const;

        /**
         * Sets the current term to the given value, and persists it to stable
         * storage.
         *
         * @param value the new current term
         */
        void set_current_term(int value);

        /**
         * Returns the candidate id that received a vote in the current term
         * (or "" if none).
         *
         * @return the candidate id that received a vote
         */
        const string& voted_for() const;

        /**
         * Sets the candidate id that received a vote in the current term to the
         * given value, and persists it to stable storage. Set to "" to indicate
         * that no candidate has been voted for in the current term.
         *
         * @param value the new candidate id that received a vote
         */
        void set_voted_for(const string& value);

    private:
        /**
         * Persist the storage state to disk. This method blocks until the data
         * is persisted to disk. This should be called by all the setters in
         * this class.
         */
        void Init();
        void Save();

        string storage_path;
        StorageMessage storage_message;
};
