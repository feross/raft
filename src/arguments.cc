#include "arguments.h"

using namespace std;

void Arguments::RegisterBool(string name, string description) {
    bool_args[name] = false;
    descriptions[name] = description;
}

void Arguments::RegisterInt(string name, string description) {
    int_args[name] = 0;
    descriptions[name] = description;
}

void Arguments::RegisterString(string name, string description) {
    string_args[name] = "";
    descriptions[name] = description;
}

void Arguments::Parse(int argc, char* argv[]) {
    vector<string> arguments(argv + 1, argv + argc);
    for (int i = 0; i < arguments.size(); i++) {
        string arg = arguments[i];

        // Named arguments are prefixed with two dashes (e.g. --mybool)
        string prefix("--");
        if (arg.substr(0, 2) == prefix) {
            string name = arg.substr(prefix.size());
            if (bool_args.count(name)) {
                bool_args[name] = true;
            } else if (int_args.count(name) || string_args.count(name)) {
                // Integer and string arguments consume the next token to
                // determine the argument value
                i += 1;
                if (i >= arguments.size()) {
                    throw MissingArgumentException();
                }
                string value = arguments[i];
                if (int_args.count(name)) {
                    int_args[name] = stoi(value);
                } else {
                    string_args[name] = value;
                 }
            } else {
                throw InvalidArgumentException();
            }
        } else {
            // Unnamed arguments have no dash prefix
            unnamed_args.push_back(arg);
        }
    }
}

bool Arguments::get_bool(string name) {
    if (!bool_args.count(name)) {
        throw InvalidArgumentException();
    }
    return bool_args[name];
}

int Arguments::get_int(string name) {
    if (!int_args.count(name)) {
        throw InvalidArgumentException();
    }
    return int_args[name];
}

const string& Arguments::get_string(string name) {
    if (!string_args.count(name)) {
        throw InvalidArgumentException();
    }
    return string_args[name];
}

const vector<string>& Arguments::get_unnamed() {
    return unnamed_args;
}

string Arguments::get_help_text() {
    string result;

    if (intro.size()) {
        result += intro + "\n";
    }

    result += "Usage:\n\n";

    for(auto const& [name, description] : descriptions) {
        string type = bool_args.count(name)
            ? "bool"
            : int_args.count(name)
                ? "int"
                : "string";
        result += "  --" + name + "\t" + description + "\t[" + type + "]\n";
    }
    return result;
}
