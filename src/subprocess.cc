// From CS110 implementation

#include "subprocess.h"

static void createPipe(int fds[]) {
  if (pipe(fds) == -1)
    throw SubprocessException("An attempt to create pipe failed.");
}

static int createProcess() {
  int pid = fork();
  if (pid == -1)
    throw SubprocessException("Failed to create a new process using fork().");
  return pid;
}

static void closeEndpoint(int endpoint) {
  if (endpoint == kNotInUse) return;
  if (close(endpoint) == -1)
    throw SubprocessException("The file descriptor " + to_string(endpoint) + " could not be closed.");
}

static void overwriteDescriptor(int newfd, int oldfd) {
  if (dup2(newfd, oldfd) == -1)
    throw SubprocessException("Failed to donate resources of descriptor " + to_string(newfd) + " to " + to_string(oldfd) + ".");
}

subprocess_t subprocess(char *argv[],
                        bool supplyChildInput, bool ingestChildOutput) {

  int pipes[4] = { kNotInUse, kNotInUse, kNotInUse, kNotInUse };
  if (supplyChildInput) createPipe(pipes);
  if (ingestChildOutput) createPipe(pipes + 2);
  subprocess_t proc = { createProcess(), pipes[1], pipes[2] };
  if (proc.pid == 0) {
    closeEndpoint(pipes[1]);
    closeEndpoint(pipes[2]);
    if (supplyChildInput) overwriteDescriptor(pipes[0], STDIN_FILENO);
    if (ingestChildOutput) overwriteDescriptor(pipes[3], STDOUT_FILENO);
    closeEndpoint(pipes[0]);
    closeEndpoint(pipes[3]);
    execvp(argv[0], argv); // returning -1 would be a user error we shouldn't identify as an exceptional error
    fprintf(stderr, "Command not found: %s\n", argv[0]); // no error checking to be done
    exit(0);
  }

  closeEndpoint(pipes[0]);
  closeEndpoint(pipes[3]);
  return proc;
}
