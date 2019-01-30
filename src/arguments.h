#ifndef _RAFT_ARGUMENTS_H_
#define _RAFT_ARGUMENTS_H_

#include <exception>
#include <map>
#include <string>
#include <vector>

using namespace std;

class InvalidArgumentException : public exception {
    const char* what() const noexcept {
        return "Invalid argument name";
    }
};

class MissingArgumentException : public exception {
    const char* what() const noexcept {
        return "Missing required argument value";
    }
};

class Arguments {
    public:
        /**
         * Command line argument parser.
         *
         * This class exposes a friendly interface for parsing a command line
         * argument string (i.e. char* argv[]) into a more useful structure. The
         * user must specify expected argument names, argument descriptions, and
         * expected argument types (bool, int, or string).
         *
         * Example:
         *
         *      int main(int argc, char* argv[]) {
         *          Arguments args("Hello World - A hello world CLI program");
         *          args.register_bool("help", "Print help message", false);
         *          try {
         *              args.parse(argc, argv);
         *          } catch (exception& err) {
         *              cerr << "Error: " << err.what() << endl;
         *              return 1;
         *          }
         *          if (args.get_bool("help")) {
         *              cout << args.get_help_text() << endl;
         *              return 0;
         *          }
         *      }
         *
         * @param intro Help text to describe the purpose of the program
         */
        Arguments(string intro) : intro(intro) {};

        /**
         * Register a named boolean command line argument with the given name
         * and description. Defaults to false.
         *
         * Boolean argument flags appear alone with no following value
         * (e.g. "--mybool").
         *
         * @param name        The full name (e.g. --mybool) of the argument
         * @param description Friendly description of the argument's purpose
         */
        void RegisterBool(string name, string description);

        /**
         * Register a named integer command line argument with the given name
         * and description. Dsefaults to 0.
         *
         * Integer argument flags must be followed immediately by a space and
         * number (e.g. "--myint 42").
         *
         * @param name        The full name (e.g. --myint) of the argument
         * @param description Friendly description of the argument's purpose
         */
        void RegisterInt(string name, string description);

        /**
         * Register a named string command line argument with the given name
         * and description. Defaults to "".
         *
         * String arguments must be followed immediately by a space and a string
         * (e.g. "--mystring hello").
         *
         * @param name        The full name (e.g. --mystring) of the argument
         * @param description Friendly description of the argument's purpose
         */
        void RegisterString(string name, string description);

        /**
         * Parse the user-provided command line argument string, usually
         * obtained directly from the arguments to main().
         *
         * May throw an exception if the command line argument string is
         * malformed, or missing a required value (e.g. for a named string or
         * integer argument which requires a value).
         *
         * @throw MissingArgumentException
         * @throw InvalidArgumentException
         *
         * @param argc Number of command line arguments
         * @param argv Array of command line argument strings
         */
        void Parse(int argc, char* argv[]);

        /**
         * Return the value of the boolean argument with the given name.
         *
         * @throw InvalidArgumentException
         *
         * @param  argument name
         * @return argument value
         */
        bool get_bool(string name);

        /**
         * Return the value of the integer argument with the given name.
         *
         * @throw InvalidArgumentException
         *
         * @param  argument name
         * @return argument value
         */
        int get_int(string name);

        /**
         * Return the value of the string argument with the given name.
         *
         * @throw InvalidArgumentException
         *
         * @param  argument name
         * @return argument value
         */
        const string& get_string(string name);

        /**
         * Return a vector of the unnamed "extra" arguments included in the
         * command line argument string.
         *
         * Unnamed arguments are useful for command line programs which accept
         * an unbounded number of command line arguments, often as the last
         * arguments to a program.
         *
         * Unnamed arguments lack the two dash prefix (--) which distinguishes
         * named arguments (e.g. --mybool) and are not associated with named
         * arguments as their value (e.g. in "--myint 42", the "42" is not an
         * unnamed argument).
         *
         * Example of 3 unnamed arguments:
         *
         *   "./program --mybool --myint 42 unnamed1 unnamed2 unnamed3"
         *
         * @return vector of unnamed argument strings
         */
        const vector<string>& get_unnamed();

        /**
         * Returns the program's help text, including the "intro" string
         * specified in the constructor, as well as a generated list of
         * argument names, descriptions, and types.
         *
         * @return string of command line help text
         */
        string get_help_text();

    private:
        map<string, string> descriptions;

        map<string, bool> bool_args;
        map<string, int> int_args;
        map<string, string> string_args;
        vector<string> unnamed_args;

        string intro;
};

#endif
