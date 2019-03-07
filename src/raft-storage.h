/**
 * This class exposes a friendly interface for getting or setting the
 * persistent server state for a Raft server. All setter methods in this
 * class ensure that data is persisted to stable storage before
 * returning.
 */

#pragma once

#include <fstream>
#include <cstdio>

#include "log.h"
#include "storage-message.pb.h"
#include "util.h"

using namespace proto;
using namespace std;

const string STORAGE_NAME_SUFFIX = "-storage.dat";

class RaftStorageException : public exception {
    public:
        RaftStorageException(const string& message): message(message) {}
        RaftStorageException(const char* message): message(message) {}
        const char* what() const noexcept { return message.c_str(); }
    private:
        string message;
};

class RaftStorage {
    public:
        /**
         * Construct a persistent server state for Raft servers, backed by
         * stable storage.
         *
         * @param storage_path Path to the storage file to use
         */
        RaftStorage(string storage_path);

        /**
         * Load data from the storage file. Throw if the file does not exist.
         *
         * @throw RaftStorageException
         */
        void Load();

        /**
         * Delete and recreate the storage file.
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
         * Returns the candidate id that received a vote in the current term
         * (or -1 if none).
         *
         * @return the candidate id that received a vote
         */
        int voted_for() const;

        /**
         * Sets the current term and the candidate id that received a vote to
         * the given values, and persists them to stable storage. Set the
         * `voted_for` parameter to -1 to indicate that no candidate has been
         * voted for in the current term.
         *
         * This combined setter is necessary to ensure that the term and
         * voted_for variables are updated and persisted to stable storage
         * in an atomic manner.
         *
         * @param current_term the new candidate id that received a vote
         * @param voted_for the candidate id that received a vote
         */
        void set_term_and_voted(int current_term, int voted_for);

        /**
         * Returns the log index number of the last log entry that has been
         * applied to the Raft state machine.
         *
         * @return log index number of the last log entry
         */
        int last_applied() const;

        /**
         * Sets the index number of the last log entry that has been applied to
         * the Raft state machine. Defaults to 0 initially.
         */
        void set_last_applied(int value);

    private:
        /**
         * Persist the storage state to disk. This method blocks until the data
         * is persisted to disk. This should be called by all the setters in
         * this class.
         *
         * @throw RaftStorageException
         */
        void Save();

        string storage_path;
        StorageMessage storage_message;
};
