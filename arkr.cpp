#include <iostream>
#include <stdio.h>
#include <stdexcept>
#include <array>
#include <memory>
#include <sstream>
#include <string>

using namespace std;

string get_mirlink() {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen("cat /etc/arkr/mirlink | tr -d '\n'", "r"), pclose); /* http://example.com/packages/arch/ */
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void print_file_content(const string &filepath) {
    string command = "cat " + filepath;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw runtime_error("Failed to run command: " + command);
    }
    array<char, 128> buffer;
    string content;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        content += buffer.data();
    }

    stringstream ss(content);
    string line;
    while (getline(ss, line)) {
        cout << line << endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <in|out> <package_name...>" << endl;
        return 1;
    }

    string setmirlink = get_mirlink();
    string group, packagename;
    int action = 0; /* 0 = help, 1 = install, 2 = uninstall */

    if (string(argv[1]) == "in") {
        action = 1;
    } else if (string(argv[1]) == "out") {
        action = 2;
    } else {
        cerr << "Invalid action: " << argv[1] << endl;
        return 1;
    }
    string command;
    for (int i = 2; i < argc; ++i) {
        if (action == 1) {
            string whatpack = argv[i];
            if (whatpack.find('/') != string::npos) {
                stringstream ss(whatpack);
                getline(ss, group, '/');
                getline(ss, packagename, '/');

                command = "wget -q --show-progress " + setmirlink + group + "/" + packagename + "/adds";
            } else {
                packagename = whatpack;
                group = ""; // Or handle it as needed

                command = "wget -q --show-progress " + setmirlink + packagename + "/adds";
            }
            unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
            if (!pipe) {
                throw runtime_error("Command to get files failed!\n"  + command);
            }
            array<char, 128> buffer;
            string adds;
            while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
                adds += buffer.data();
            }
            cout << adds << endl;

            print_file_content("adds");
        }
    }

    return 0;
}
