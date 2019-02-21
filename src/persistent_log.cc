#include "persistent_log.h"


PersistentLog::PersistentLog(const char *filename) {
    std::string cursor_filename_str = std::string(filename) + "_cursor";
    cursor_filename = strdup(cursor_filename_str.c_str());
    std::string log_filename_str = std::string(filename) + "_log";
    log_filename = strdup(log_filename_str.c_str());
    debug("cursor filename: %s , log: %s", cursor_filename, log_filename);
    log_file = NULL;
    cursor_file = NULL;
    bool success = ReopenLog();
    if (!success) {
        warn("failed to reopen log %s", filename);
        return;
    }
}


PersistentLog::~PersistentLog() {
    RemoveCachedLogEntries();
    delete(cursor_filename);
    delete(log_filename);
}

bool PersistentLog::ResetLog() {
    cursor = 0;
    RemoveCachedLogEntries();
    log_entries.clear();

    if(Util::PersistentFileUpdate(cursor_filename, &cursor, sizeof(int)) == false) {
        warn("Error: PersistentFileUpdate failed to write to cursor %d file", cursor);
        return false;
    }
    log_file = fopen( log_filename , "wb" ); //erases old log
    fclose(log_file);

    log_file = fopen( log_filename , "r+b" );
    cursor_file = fopen( cursor_filename, "r+b" );
    if (log_file == NULL || cursor_file == NULL) {
        warn("Error: failed to open log || cursor file %s", cursor_filename);
        return false;
    }
    int zero = 0;
    char base_entry[10 + sizeof(int)];
    memcpy(base_entry, &zero, sizeof(int));
    memcpy(base_entry + 4, "echo hell\0", 10);
    AddLogEntry(base_entry, 10 + sizeof(int)); //start of all logs is same
    AddLogEntry(base_entry, 10 + sizeof(int)); //need previous entry too
    return true;
}

bool PersistentLog::ReopenLog() {
    cursor_file = fopen( cursor_filename , "r+b" );
    log_file = fopen( log_filename , "r+b" );

    if (cursor_file == NULL || log_file == NULL) {
        debug("creating cursor & log files %s", log_filename);
        if (cursor_file != NULL) fclose(cursor_file);
        if (log_file != NULL) fclose(log_file);
        if (ResetLog() != true) {
            debug("%s", "Failed to reset log");
        }
    }
    fread(&cursor, sizeof(int), 1, cursor_file);
    debug("opened log, cursor: %d", cursor);
    fclose(cursor_file);
    if (LoadIndexFromLog() != true) {
        warn("Failed to load full log into memory %s", log_filename);
        return false;
    }
    return true;
}


bool PersistentLog::LoadIndexFromLog() {
    if (cursor == 0) return true;
    log_entries.clear();
    int scan_location = 0;
    while(true) {
        int success = fseek(log_file, scan_location, SEEK_SET);
        if (success != 0) {
            warn("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
            return false;
        }
        int entry_size;
        int read_bytes = fread( &entry_size, 1, sizeof(int), log_file);
        if (read_bytes != sizeof(int)) {
            warn("Error: fread failed to read int bytes from log file at scan_location %d, cursor %d, %s (%d)", scan_location, cursor, strerror(errno), errno);
            return false;
        }
        struct LogEntry current_entry;
        current_entry.data = NULL;
        current_entry.offset = scan_location;
        current_entry.len = entry_size;
        log_entries.push_back(current_entry);

        scan_location += ((sizeof(int) * 2) + entry_size);
        if (scan_location == cursor) return true;
    }
}


void PersistentLog::RemoveCachedLogEntries() {
    for (int i = 0; i<log_entries.size(); i++) {
        if (log_entries[i].data != NULL) {
            delete(log_entries[i].data ); //free lazily populated in-memory log entries
        }
    }
}


bool PersistentLog::MoveCursor(int distance) {
    int new_cursor_position = cursor + distance;
    if (Util::PersistentFileUpdate(cursor_filename, &new_cursor_position, sizeof(int))) {
        cursor = new_cursor_position;
        debug("moved cursor to %d", cursor);
        return true;
    }
    warn("Error: failed to update cursor location, cursor: %d", cursor);
    return false;
}


bool PersistentLog::AddLogEntry(const void* log_data, int log_data_len) {
    struct LogEntry current_entry;
    current_entry.data = NULL;
    current_entry.len = log_data_len;
    current_entry.offset = cursor;

    int success = fseek(log_file, cursor, SEEK_SET); // to make sure we're in right location
    if (success != 0) {
        warn("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
        return false;
    }
    int wrote_bytes = fwrite(&log_data_len, 1, sizeof(int), log_file);
    if (wrote_bytes != sizeof(int)) {
        warn("Error: fwrite failed to write int bytes to log file, %s (%d)", strerror(errno), errno);
        return false;
    }
    wrote_bytes = fwrite(log_data, 1, log_data_len, log_file);
    if (wrote_bytes != log_data_len) {
        warn("Error: fwrite failed to write log_data_len bytes to log, %s (%d)", strerror(errno), errno);
        return false;
    }
    wrote_bytes = fwrite(&log_data_len, 1, sizeof(int), log_file);
    if (wrote_bytes != sizeof(int)) {
        warn("Error: fwrite failed to write int bytes to log file, %s (%d)", strerror(errno), errno);
        return false;
    }
    success = fflush(log_file);
    if (success != 0) {
        warn("Error: fflush failed to push all writes to disk, %s (%d)", strerror(errno), errno);
        return false;
    }

    success = MoveCursor(log_data_len + (sizeof(int) * 2));
    if (!success) {
        warn("failed to move cursor safely, %s (%d)", strerror(errno), errno);
        return false;
    }
    log_entries.push_back(current_entry);
    return true;
}


bool PersistentLog::RemoveLogEntry() {
    int prev_entry_size;
    int success = fseek( log_file, cursor - sizeof(int), SEEK_SET);
    if (success != 0) {
        warn("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
        return false;
    }
    int read_bytes = fread( &prev_entry_size, 1, sizeof(int), log_file);
    if (read_bytes != sizeof(int)) {
        warn("Error: fread failed to read int bytes from log file, %s (%d)", strerror(errno), errno);
        return false;
    }

    bool moved = MoveCursor(-1 * (prev_entry_size + (sizeof(int) * 2)));
    if (!moved) {
        warn("Failed to back up cursor, entry not deleted. cursor: %d", cursor);
        return false;
    }
    //free if we had saved log into memory for client use
    struct LogEntry delete_entry = log_entries.back();
    if (delete_entry.data != NULL) {
        delete(delete_entry.data);
    }
    log_entries.pop_back();
    return true;
}


const struct LogEntry PersistentLog::GetLogEntryByIndex(int index) {
        if (log_entries.size() <= index) {
            debug("index %d is not in log, too large", index);
            struct LogEntry current_entry;
            current_entry.data = NULL;
            current_entry.len = -1;
            current_entry.offset = -1;
            return current_entry;
        }
        struct LogEntry current_entry = log_entries[index];
        if (current_entry.data != NULL) {
            debug("Entry already loaded : %s", current_entry.data);
            return current_entry;
        }
        int success = fseek(log_file, current_entry.offset + sizeof(int), SEEK_SET); // to make sure we're in right location
        if (success != 0) {
            warn("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
            return current_entry; // NULL data
        }

        char *buffer = new char[current_entry.len + 1];
        buffer[current_entry.len] = '\0';
        int read_bytes = fread( buffer, 1, current_entry.len, log_file);
        if (read_bytes != current_entry.len) {
            warn("Error: fread failed to read int bytes from log file, %s (%d)", strerror(errno), errno);
            return current_entry; // NULL data
        }
        debug("Entry Loaded : %s", buffer);
        current_entry.data = buffer;
        log_entries[index] = current_entry;
        return current_entry;
}

int PersistentLog::LastLogIndex() {
    return log_entries.size() - 1;
}


