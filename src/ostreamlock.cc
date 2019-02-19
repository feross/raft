// Implementation from CS110
// /usr/class/cs110/local/src/threads/ostreamlock.cc

#include "ostreamlock.h"

using namespace std;

static mutex mapLock;
static map<ostream *, unique_ptr<mutex>> streamLocks;


// TODO: Thought: maybe instead make a "log" function that has printf syntax

ostream& oslock(ostream& os) {
    ostream *ostreamToLock = &os;
    if (ostreamToLock == &cerr) ostreamToLock = &cout; //ditto
    mapLock.lock();
    unique_ptr<mutex>& up = streamLocks[ostreamToLock];
    if (up == nullptr) {
        up.reset(new mutex);
    }
    mapLock.unlock();
    up->lock();
    return os;
}

ostream& osunlock(ostream& os) {
    ostream *ostreamToLock = &os;
    if (ostreamToLock == &cerr) ostreamToLock = &cout; //TODO: should comment (using one lock for stdout/err)
    mapLock.lock();
    auto found = streamLocks.find(ostreamToLock);
    mapLock.unlock();
    if (found == streamLocks.end()) {
        throw "unlock inserted into stream that has never been locked."; //TODO: have an exception type
    }
    unique_ptr<mutex>& up = found->second;
    up->unlock();
    return os;
}
