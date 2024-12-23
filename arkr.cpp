#include <iostream>
#include <stdio.h>
#include <stdexcept>
#include <fstream>
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include "json.hpp"
#include <unistd.h>
#include <ctime>
using json = nlohmann::json_abi_v3_11_3::json;
using namespace std;
const string version = "rolling";
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
    json arkrjson;


    json::array_t packlist;
    ifstream arkrjsonfile("/etc/arkr.json");
    if (!arkrjsonfile.is_open()) {
        cerr << "Failed to open arkr.json! Does it exist?" << endl;
    }
    arkrjsonfile >> arkrjson;
    arkrjsonfile.close();
    if (arkrjson.contains("packages") && arkrjson["packages"].is_array()) {
        packlist = arkrjson["packages"];
    } else { cerr << "JSON does not contain an array named 'packages'!" << endl; return 1; }
    

    string setmirlink = get_mirlink();
    string group, packagename;
    int action = 0; /* 0 = help, 1 = install, 2 = uninstall */

    if (string(argv[1]) == "in") {
        action = 1;
        if (geteuid() != 0) {
        cerr << "This program must be run as root, please su, sudo or doas to run this program" << endl;
        return 1;
    }
    } else if (string(argv[1]) == "out") {
        action = 2;
        if (geteuid() != 0) {
        cerr << "This program must be run as root, please su, sudo or doas to run this program" << endl;
        return 1;
    }
    } else if (string(argv[1]) == "about") {
        // Get compile time
        const char* compileDate = __DATE__;
        const char* compileTime = __TIME__;

        // Parse compile date and time
        int month, day, year, hour, minute, second;
        sscanf(compileDate, "%*s %d %d", &day, &year);
        string monthStr = string(compileDate).substr(0, 3);

        // Convert month string to number
        if (monthStr == "Jan") month = 1;
        else if (monthStr == "Feb") month = 2;
        else if (monthStr == "Mar") month = 3;
        else if (monthStr == "Apr") month = 4;
        else if (monthStr == "May") month = 5;
        else if (monthStr == "Jun") month = 6;
        else if (monthStr == "Jul") month = 7;
        else if (monthStr == "Aug") month = 8;
        else if (monthStr == "Sep") month = 9;
        else if (monthStr == "Oct") month = 10;
        else if (monthStr == "Nov") month = 11;
        else if (monthStr == "Dec") month = 12;

        // Parse time
        sscanf(compileTime, "%d:%d:%d", &hour, &minute, &second);

        // Format time string
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d%02d%04d%02d%02d%02d", day, month, year, second, minute, hour);

        // Print version and compile time
        cout << "Version: " << version << endl;
        cout << "Build date: " << buffer << endl;
        return 0;
    } else {
        cerr << "Invalid action: " << argv[1] << endl;
        return 1;
    }
    string command;
    string catcommand;
    for (int i = 2; i < argc; ++i) {
        if (action == 1) {
            packlist.push_back(argv[i]);
            string whatpack = argv[i];
            if (whatpack.find('/') != string::npos) {
                stringstream ss(whatpack);
                getline(ss, group, '/');
                getline(ss, packagename, '/');

                command = "wget -q " + setmirlink + group + "/" + packagename + "/adds";
                catcommand = "wget -q --show-progress " + setmirlink + group + "/" + packagename + "/";
            } else {
                packagename = whatpack;
                group = "";

                command = "wget -q " + setmirlink + packagename + "/adds";
                catcommand = "wget -q --show-progress " + setmirlink + packagename + "/";
            }

            int ret = system(command.c_str());
            if (ret != 0) {
                throw runtime_error("Command to get files failed!\n" + command);
            }

            string catcmd = "cat adds";
            cout << "Binaries to download:" << endl;
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
            system(("wget -q " + setmirlink + packagename + "/optional.json").c_str());
	        cout << "Optional packages:" << endl;
            system("cat optional.json"); cout << endl;
            stringstream ss(content);
            string line;
            while (getline(ss, line)) {
                string download_command = catcommand + line;
		        cout << "Downloading " << line << endl;
                ret = system(download_command.c_str());
                if (ret != 0) {
                    throw runtime_error("Failed to run command: " + download_command);
                }
            };
            ss.clear();
            ss.seekg(0,ss.beg);
            while (getline(ss, line)) {
		        string chmodcommand = "chmod 755 " + line;
                cout << chmodcommand << endl;
                ret = system(chmodcommand.c_str());
                if (ret != 0) {
                    throw runtime_error("Failed to run command: " + chmodcommand);
                }
	        }
        } else if (action==2) {
            packlist.erase(remove(packlist.begin(), packlist.end(), argv[i]));
        }
    }
    if (arkrjson["packages"]==packlist){
        cout << "No changes done to packlist." << endl;
    } else {
        arkrjson["packages"]=packlist;
        ofstream outputarkrjson("/etc/arkr.json");
        if (!outputarkrjson.is_open()) {
            cerr << "Cannot open arkr.json!!!" << endl;
            return 1;
        }
        outputarkrjson << setw(4) << arkrjson << endl;
        outputarkrjson.close();
        cout << "Wrote new package list" << endl;
    }
    return 0;
}
