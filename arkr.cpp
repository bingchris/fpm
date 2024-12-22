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
    string catcommand;
    for (int i = 2; i < argc; ++i) {
        if (action == 1) {
            string whatpack = argv[i];
            if (whatpack.find('/') != string::npos) {
                stringstream ss(whatpack);
                getline(ss, group, '/');
                getline(ss, packagename, '/');

                command = "wget -q --show-progress " + setmirlink + group + "/" + packagename + "/adds";
                catcommand = "wget -q --show-progress " + setmirlink + group + "/" + packagename + "/";
            } else {
                packagename = whatpack;
                group = ""; // Or handle it as needed

                command = "wget -q --show-progress " + setmirlink + packagename + "/adds";
                catcommand = "wget -q --show-progress " + setmirlink + packagename + "/";
            }

            // Run the wget command and wait for it to complete
            int ret = system(command.c_str());
            if (ret != 0) {
                throw runtime_error("Command to get files failed!\n" + command);
            }

            // Run the cat command to read the content of the adds file
            string catcmd = "cat adds";
            ret = system(catcmd.c_str());
            if (ret != 0) {
                throw runtime_error("Failed to run command: " + catcmd);
            }

            array<char, 128> buffer;
            string content;
            FILE* file = fopen("adds", "r");
            if (!file) {
                throw runtime_error("Failed to open adds file!");
            }
            while (fgets(buffer.data(), buffer.size(), file) != nullptr) {
                content += buffer.data();
            }
            fclose(file);

            stringstream ss(content);
            string line;
            while (getline(ss, line)) {
                string download_command = catcommand + line;
                ret = system(download_command.c_str());
                if (ret != 0) {
                    throw runtime_error("Failed to run command: " + download_command);
                }
                string chmodcommand = "chmod 755 " + line;
                cout << chmodcommand << endl;
                ret = system(chmodcommand.c_str());
                if (ret != 0) {
                    throw runtime_error("Failed to run command: " + chmodcommand);
                }
            }
        }
    }

    return 0;
}
