#pragma once

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// #include "log.h"

/**
 * Struct describing a particular log entry, including:
 * - the content of the log entry
 * - the length of that content
 * - the byte-offset into log where the data associated with this entry begins
 */
struct LogEntry {
  char *data;
  int len;
  int offset;
};

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
       * Returns the index-th entry in the log, if present.  If not present,
       * returns a log entry with -1 size & NULL entry pointer.
       * 
       * @return LogEntry containing a pointer to the entry & its length
       */
      const struct LogEntry GetLogEntryByIndex(int index);
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
       * Loads into memory information about the log.  Because we are restoring
       * from disk, this scans over the full log.  Used for more efficient
       * manipulation of the log (to avoid constant disk seeking).
       *
       * @return bool - whether we successfully scanned the log info into memory
       */
      bool load_index_from_log();
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

      /*
       * In-memory representation of the log.  Because the log may be large, we
       * only lazily populate log content (which is subsequently tracked and
       * freed during destruction, so client doesn't have to free)
       */
      std::vector<struct LogEntry> log_entries;


};