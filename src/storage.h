#ifndef _RAFT_STORAGE_H_
#define _RAFT_STORAGE_H_

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
        int get_current_term() const;

        /**
         * Sets the current term to the given value, and persists it to stable
         * storage.
         *
         * @param value the new current term
         */
        void set_current_term(int value);

        /**
         * Returns the candidate id that received a vote in the current term
         * (or 0 if none).
         *
         * @return the candidate id that received a vote
         */
        int get_voted_for() const;

        /**
         * Sets the candidate id that received a vote in the current term to the
         * given value, and persists it to stable storage. Set to 0 to indicate
         * that no candidate has been voted for in the current term.
         *
         * @param value the new candidate id that received a vote
         */
        void set_voted_for(int value);

        /**
         * Returns all log entries in a vector. Each log entry contains a command
         * for the state machine. (first index is 1)
         *
         * TODO: Each log entry should contain the term when the entry was
         * received by leader.
         *
         * @return vector of log entries
         */
        vector<string> get_log () const;

        /**
         * Returns the log entry element at the given zero-based index. Calling
         * this method with index outside of [0, log_size()) yields undefined
         * behavior.
         *
         * @param  index the index of the log entry to return
         * @return the log entry
         */
        const string& get_log (int index) const;

        /**
         * Returns the number of log entries currently in the log.
         *
         * @return integer number of log entries
         */
        int log_size() const;

        /**
         * Sets the value of the log entry element at the given zero-based
         * index, and persists it to stable storage.
         *
         * @param index the index of the log entry to set
         * @param value the new value to set
         */
        void set_log(int index, const string& value);

        /**
         * Appends a new log entry element to the log with the given value, and
         * persists it to stable storage
         *
         * @param value the new log entry to add
         */
        void add_log(const string& value);

    private:
        void Init();
        void Save();

        string storage_path;
        proto::Storage storage_message;
};

#endif
