// Implementation from CS110
// /usr/class/cs110/local/include/ostreamlock.h

#pragma once

#include <ostream>
#include <iostream>
#include <mutex>
#include <map>

/**
 * Two stream manipulators designed to make insertion into ostreams thread-safe.
 *
 * Provides a locking mechanism to lock down access to cout, cerr, and other
 * stream references so that the full accumulation of daisy-chained data is
 * inserted into an ostream as one big insertion.
 *
 * Examples:
 *     Thread safe:
 *         cout << oslock << 1 << 2 << 3 << 4 << endl << osunlock;
 *
 *     Not thread safe:
 *         cout << 1 << 2 << 3 << 4 << endl;
 */
std::ostream& oslock(std::ostream& os);
std::ostream& osunlock(std::ostream& os);
