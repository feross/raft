class PersistentLog {
    public:
      /*
       * Reloads a persistent log with the prefix specified by filename.  If
       * none is present, creates 2 files: filename + _log & filename + _cursor
       * and initializes the log to be empty.
       */
      PersistentLog(const char *filename);
      /*
       * Cleans up temporary information used by persistent log for performance,
       * but leaves the persistent log intacted.
       */
      ~PersistentLog();
      /*
       * Appends an entry to the end of our persistent log.
       * 
       * @param log_data - pointer to the data to be appended
       * @param log_data_len - length of the data to be appended
       *
       * @return bool - true if successfully appended to persistent log
       */    
      bool AddLogEntry(const void* log_data, int log_data_len);
      /*
       * Removes the last entry from the end of our persistent log.
       *
       * @return bool - true if successfully removed entry from persistent log
       */  
      bool RemoveLogEntry();

    private:

      /*
       * Reopens a previously closed log, e.g. on startup.  If none is present,
       * creates the necessary files & initializes the log to be empty.
       * 
       * @return bool - true if successfully reopened/created the persistent log
       */
      bool ReopenLog();
      /*
       * Persistently updates the cursor file and cursor, effectively adding
       * or removing bytes from the file.  Cursor specifies the active "end"
       * of the log, any subsequent bytes are garbage
       * 
       * @return bool - whether we successfully & persistently moved the cursor
       */
      bool move_cursor(int distance);

      /*
       * Name of persistent cursor file, necessary to enable safe adding/removing
       * from the file
       */
      const char *cursor_filename;

      /*
       * Name of persistent log file, which contains the log entries & metadata
       */
      const char *log_filename;
      /*
       * A handle to the persistent log file, should be open during normal
       * operation to avoid overhead of closing/opening constantly.
       */ 
      FILE *log_file;
      /*
       * A handle to the persistent cursor file, may be open during normal
       * operation, though regularly closed to atomically move cursor
       */ 
      FILE *cursor_file;
      /*
       * Current cursor location, should match cursor_filename body at any time
       * other than initialization or in the midst of a crash.
       */
      int cursor;
};