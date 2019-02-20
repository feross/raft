#include "persistent_log.h"


bool SyscallErrorInfo(bool syscall_success, const char *error_message_prefix) {
  if (!syscall_success) {
    printf("Error: %s, %s (%d)\n", error_message_prefix, strerror(errno), errno);
  }
  return syscall_success;
}


bool PersistentFileUpdate(const char * filename, void * new_contents, int new_contents_len) {
  // assume old file is safe
  std::string tmp_filename = "tmp_" + std::string(filename);
  printf("tmp name: %s\n", tmp_filename.c_str());
  FILE * tmp_file = fopen( tmp_filename.c_str() , "wb" );
  bool opened = SyscallErrorInfo(tmp_file != NULL,
    "fopen of temp file as 'wb' failed"); //TODO: include filename
  bool written = SyscallErrorInfo(new_contents_len == fwrite(new_contents, 1, new_contents_len, tmp_file),
    "fwrite of new_contents to tmp_file failed"); //TODO include more info
  bool flushed = SyscallErrorInfo(0 == fflush(tmp_file),
    "Error: fflush failed to push all writes to disk, ");
  bool closed = SyscallErrorInfo(0 == fclose(tmp_file), "fclose of tmp_file failed"); //TODO include filename
  if (opened && written && closed && flushed) {
    return SyscallErrorInfo(0 == rename(tmp_filename.c_str(), filename),
    "rename of tmp_filename to filename failed"); //TODO: include more info
  }
  printf("File Update Failed %s, %s, %d", filename, new_contents, new_contents_len);
  return false;
}


PersistentLog::PersistentLog(const char *filename) {
  std::string cursor_filename_str = std::string(filename) + "_cursor";
  cursor_filename = strdup(cursor_filename_str.c_str());
  std::string log_filename_str = std::string(filename) + "_log";
  log_filename = strdup(log_filename_str.c_str());
  printf("cursor filename: %s , log: %s\n", cursor_filename, log_filename);
  log_file = NULL;
  cursor_file = NULL;
  bool success = ReopenLog();
  if (!success) {
    printf("failed to reopen log %s\n", filename);
    return;
  }
}


PersistentLog::~PersistentLog() {
  for (int i = 0; i<log_entries.size(); i++) {
    if (log_entries[i].data != NULL) {
      delete(log_entries[i].data ); //free lazily populated in-memory log entries
    }
  }
  delete(cursor_filename);
  delete(log_filename);
}


bool PersistentLog::ReopenLog() {
  cursor_file = fopen( cursor_filename , "r+b" );
  log_file = fopen( log_filename , "r+b" );

  if (cursor_file == NULL || log_file == NULL) {
    printf("creating cursor & log files\n");
    if (cursor_file != NULL) fclose(cursor_file);
    if (log_file != NULL) fclose(log_file);
    cursor = 0;

    if(PersistentFileUpdate(cursor_filename, &cursor, sizeof(int)) == false) {
      printf("Error: PersistentFileUpdate failed to write to cursor file\n");
      return false;
    }
    log_file = fopen( log_filename , "wb" );
    fclose(log_file);

    log_file = fopen( log_filename , "r+b" );
    cursor_file = fopen( cursor_filename, "r+b" );
    if (log_file == NULL || cursor_file == NULL) {
      printf("Error: failed to open log || cursor file");
      return false;
    }
  }
  fread(&cursor, sizeof(int), 1, cursor_file);
  printf("opened log, cursor: %d\n", cursor);
  fclose(cursor_file);
  if (load_index_from_log() != true) {
    printf("Failed to load full log into memory");
    return false;
  }
  return true;
}


bool PersistentLog::load_index_from_log() {
  if (cursor == 0) return true;
  log_entries.clear();
  int scan_location = 0;
  while(true) {
    int success = fseek(log_file, scan_location, SEEK_SET);
    if (success != 0) {
      printf("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
      return false;
    }
    int entry_size;
    int read_bytes = fread( &entry_size, 1, sizeof(int), log_file);
    if (read_bytes != sizeof(int)) {
      printf("Error: fread failed to read int bytes from log file at scan_location %d, cursor %d, %s (%d)", scan_location, cursor, strerror(errno), errno);
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


bool PersistentLog::move_cursor(int distance) {
  int new_cursor_position = cursor + distance;
  if (PersistentFileUpdate(cursor_filename, &new_cursor_position, sizeof(int))) {
    cursor = new_cursor_position;
    printf("moved cursor to %d\n", cursor);
    return true;
  }
  printf("Error: failed to update cursor location");
  return false;
}


bool PersistentLog::AddLogEntry(const void* log_data, int log_data_len) {
  struct LogEntry current_entry;
  current_entry.data = NULL; 
  current_entry.len = log_data_len;
  current_entry.offset = cursor;

  int success = fseek(log_file, cursor, SEEK_SET); // to make sure we're in right location
  if (success != 0) {
    printf("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
    return false;
  }
  int wrote_bytes = fwrite(&log_data_len, 1, sizeof(int), log_file); 
  if (wrote_bytes != sizeof(int)) {
    printf("Error: fwrite failed to write int bytes to log file, %s (%d)", strerror(errno), errno);
    return false;
  }
  wrote_bytes = fwrite(log_data, 1, log_data_len, log_file);
  if (wrote_bytes != log_data_len) {
    printf("Error: fwrite failed to write log_data_len bytes to log, %s (%d)", strerror(errno), errno);
    return false;
  }
  wrote_bytes = fwrite(&log_data_len, 1, sizeof(int), log_file);
  if (wrote_bytes != sizeof(int)) {
    printf("Error: fwrite failed to write int bytes to log file, %s (%d)", strerror(errno), errno);
    return false;
  }
  success = fflush(log_file);
  if (success != 0) {
    printf("Error: fflush failed to push all writes to disk, %s (%d)", strerror(errno), errno);
    return false;
  }

  success = move_cursor(log_data_len + (sizeof(int) * 2));
  if (!success) {
    printf("failed to move cursor safely, %s (%d)", strerror(errno), errno);
    return false;
  }
  log_entries.push_back(current_entry);
  return true;
}


bool PersistentLog::RemoveLogEntry() {
  int prev_entry_size;
  int success = fseek( log_file, cursor - sizeof(int), SEEK_SET);
  if (success != 0) {
    printf("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
    return false;
  }
  int read_bytes = fread( &prev_entry_size, 1, sizeof(int), log_file);
  if (read_bytes != sizeof(int)) {
    printf("Error: fread failed to read int bytes from log file, %s (%d)", strerror(errno), errno);
    return false;
  }

  bool moved = move_cursor(-1 * (prev_entry_size + (sizeof(int) * 2)));
  if (!moved) {
    printf("Failed to back up cursor, entry not deleted");
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
      printf("index %d is not in log, too large\n", index);
      struct LogEntry current_entry;
      current_entry.data = NULL; 
      current_entry.len = -1;
      current_entry.offset = -1;
      return current_entry;
    }
    struct LogEntry current_entry = log_entries[index];
    if (current_entry.data != NULL) {
      printf("Entry already loaded : %s\n", current_entry.data);
      return current_entry;
    }
    int success = fseek(log_file, current_entry.offset + sizeof(int), SEEK_SET); // to make sure we're in right location
    if (success != 0) {
      printf("Error: fseek failed to move to move to a new file offset, %s (%d)", strerror(errno), errno);
      return current_entry; // NULL data
    }

    char *buffer = new char[current_entry.len + 1];
    buffer[current_entry.len] = '\0';
    int read_bytes = fread( buffer, 1, current_entry.len, log_file);
    if (read_bytes != current_entry.len) {
      printf("Error: fread failed to read int bytes from log file, %s (%d)", strerror(errno), errno);
      return current_entry; // NULL data
    }
    printf("Entry Loaded : %s\n", buffer);
    current_entry.data = buffer;
    log_entries[index] = current_entry;
    return current_entry;
}


