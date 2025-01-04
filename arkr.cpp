#include <iostream>
#include <stdio.h>
#include <stdexcept>
#include <fstream>
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include "json.hpp"
#include "config.hpp"
#include <unistd.h>
#include <ctime>
#include <algorithm>
using json = nlohmann::json_abi_v3_11_3::json;
using namespace std;
const string version = "rolling";
string get_mirlink() {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen("cat /etc/arkr/mirlink | tr -d '\n'", "r"), pclose); /* http://example.com/packages/arch/ */
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <in|out> <package_name...>/configure <config|dir|diruser> (arkr.json|None|usertosetup)" << endl;
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
    int confret;

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
    } else if (string(argv[1]) == "configure") {
        if (geteuid() != 0) {
        cerr << "This program must be run as root, please su, sudo or doas to run this program" << endl;
        return 1;
    } else {
        int conftype;
        if (argv[2] == "config") {conftype = 1;}
        else if (argv[2] == "dir") {conftype = 2;}
        else if (argv[2] == "diruser") {conftype = 3;}
        else {
            cerr << "Invalid config type!" << endl;
            return 1;
        }
        switch (conftype) {
            case 1:
                confret = system(("cp " + string(argv[3]) + " /etc/arkr.json").c_str()); 
                if (confret != 0) {
                    cerr << "Failed to copy " << string(argv[3]) << " to /etc/arkr.json" << endl;
                    return 1;
                }
                cout << "Copied " << string(argv[3]) << " to /etc/arkr.json" << endl;
                return 0;
            case 2:
                confret = system(("mkdir /var/cache/arkr/"));  
                if (confret != 0) {
                    cerr << "Failed to make directory /var/cache/arkr/" << endl;
                    return 1;
                }
                cout << "Setup arkr directory /var/cache/arkr/" << endl;
                return 0;
            case 3:
                if (config::Experimental::usermode == false) { 
                    cerr << "This feature is only available in usermode!" << endl;
                    return 1;
                } else {
                confret = system(("mkdir /home/" + string(argv[3]) + "/.arkr/").c_str());  
                if (confret != 0) {
                    cerr << "Failed to make directory /home/" + string(argv[3]) + "/.arkr/" << endl;
                    return 1;
                }
                cout << "Setup arkr directory /home/" + string(argv[3]) + "/.arkr/" << endl;
                return 0;
                }
        
            }
    }
    } else if (string(argv[1]) == "about") {
        // compile time and date
        const char* compileDate = __DATE__;
        const char* compileTime = __TIME__;
        // parseing
        int month, day, year, hour, minute, second;
        sscanf(compileDate, "%*s %d %d", &day, &year);
        string monthStr = string(compileDate).substr(0, 3);
        // we dont like month names
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
        // more pasreing
        sscanf(compileTime, "%d:%d:%d", &hour, &minute, &second);
        // now formateing
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d%02d%04d%02d%02d%02d", day, month, year, hour, minute, second);
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
            string packtoadd = argv[i];
            
            string whatpack = argv[i];
            if (whatpack.find('/') != string::npos) {
                stringstream ss(whatpack);
                getline(ss, group, '/');
                getline(ss, packagename, '/');
                packtoadd = packagename;
                command = "wget -q " + setmirlink + group + "/" + packagename + "/adds";
                catcommand = "wget -q --show-progress " + setmirlink + group + "/" + packagename + "/";
            } else {
                packagename = whatpack;
                group = "";
                packtoadd = packagename;
                command = "wget -q " + setmirlink + packagename + "/adds";
                catcommand = "wget -q --show-progress " + setmirlink + packagename + "/";
            }
            if (find(packlist.begin(), packlist.end(), packtoadd) == packlist.end()) {
                
            } else {
                cout << "Package " << packtoadd << " is already installed. Reinstalling." << endl;
            }
            int ret = system(command.c_str());
            if (ret != 0) {
                cerr << ("Command to get files failed!\n" + command) << endl;
                return 1;
            }

            string catcmd = "cat adds";
            cout << "Binaries to download:" << endl;
            ret = system(catcmd.c_str());
            if (ret != 0) {
                cerr << ("Failed to run command: " + catcmd) << endl;
                return 1;
            }

            array<char, 128> addsbuffer;
            string addscontent;
            FILE* addsfile = fopen("adds", "r");
            if (!addsfile) {
                cerr << ("Failed to open adds file!") << endl;
                return 1;
            }
            while (fgets(addsbuffer.data(), addsbuffer.size(), addsfile) != nullptr) {
                addscontent += addsbuffer.data();
            }
            fclose(addsfile);
            system(("wget -q " + setmirlink + packagename + "/optional.json").c_str());


            system(("wget -q " + setmirlink + packagename + "/version").c_str());
            array<char, 128> versionbuffer;
            string versioncontent;
            FILE* versionfile = fopen("version", "r");
            if (!versionfile) {
                cerr << ("Failed to open version file!") << endl;
                return 1;
            }
            while (fgets(versionbuffer.data(), versionbuffer.size(), versionfile) != nullptr) {
                versioncontent += versionbuffer.data();
            }
            fclose(versionfile);
	        cout << "Optional packages:" << endl;
            system("cat optional.json"); cout << endl;
            stringstream ss(addscontent);
            string line;
            while (getline(ss, line)) {
                string download_command = catcommand + line;
		        cout << "Downloading " << line << endl;
                ret = system(download_command.c_str());
                if (ret != 0) {
                    cerr << ("Failed to run command: " + download_command) << endl;
                    return 1;
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
	        };
            if (find(packlist.begin(), packlist.end(), packtoadd) == packlist.end()) {
                packlist.push_back(packtoadd);
                cout << "Installed " << packtoadd << endl;
            } 
            ss.clear();
            ss.seekg(0,ss.beg);
            arkrjson["packagever"][packagename] = versioncontent;
            
        } else if (action==2) {
            auto iter = remove(packlist.begin(), packlist.end(), argv[i]);
            if (iter != packlist.end()) {
                packlist.erase(iter, packlist.end());
            } else {
                cerr << "No package named that!" << endl;
                system("cat /etc/arkr.json");
            }
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
