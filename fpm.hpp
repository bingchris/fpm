#ifndef FPM_LIBRARY_HPP
#define FPM_LIBRARY_HPP

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <algorithm>
#include <unistd.h>
#include "json.hpp"
#include "config.hpp"

using json = nlohmann::json;
using namespace std;
namespace fpm {
const string version = "rolling";

string get_mirlink() {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen("cat /etc/fpm/mirlink | tr -d '\n'", "r"), pclose);
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
string findbestmirror() {
    vector<string> urls;
    string line;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen("cat /etc/fpm/mirlinks", "r"), pclose);

    if (!pipe) {
        throw runtime_error("Could not open /etc/fpm/mirlinks");
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
        line = buffer;
        line.erase(remove(line.begin(), line.end(), '\n'), line.end());
        urls.push_back(line);
    }

    vector<pair<string, double>> pingTimes; // Store URL and ping time

    for (string& url : urls) {
        // Trim the URL to get the domain
        if (url.find("https://") != string::npos) {
            url.erase(0, 8);
        } else if (url.find("http://") != string::npos) {
            url.erase(0, 7);
        }
        size_t slashPos = url.find('/');
        if (slashPos != string::npos) {
            url.erase(slashPos);
        }

        // Ping the URL
        string pingCommand = "ping -c 1 -W 1 " + url + " 2>&1"; // 2>&1 redirects errors
        unique_ptr<FILE, decltype(&pclose)> pingPipe(popen(pingCommand.c_str(), "r"), pclose);
        

        if (pingPipe) {
            char pingBuffer[128];
            string pingOutput;
            while (fgets(pingBuffer, sizeof(pingBuffer), pingPipe.get()) != nullptr) {
                pingOutput += pingBuffer;
            }

            // Extract the ping time from the output
            size_t timePos = pingOutput.find("time=");
            if (timePos != string::npos) {
                size_t msPos = pingOutput.find("ms", timePos);
                if (msPos != string::npos) {
                    string timeStr = pingOutput.substr(timePos + 5, msPos - (timePos + 5));
                    cout << url << ": " << timeStr << "ms" << endl;
                    try {
                        double pingTime = stod(timeStr);
                        pingTimes.emplace_back(url, pingTime);
                    } catch (const invalid_argument& e) {
                        cerr << "Could not parse ping time for " << url << endl;
                    }
                }
            } else {
                cerr << "Could not find ping time for " << url << endl;
            }
        } else {
            cerr << "Failed to ping " << url << endl;
        }
    }

    // Find the fastest URL
    if (pingTimes.empty()) {
        throw runtime_error("No mirrors could be reached.");
    }

    auto fastestMirror = min_element(pingTimes.begin(), pingTimes.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    return fastestMirror->first;
}

bool is_root() {
    return geteuid() == 0;
}

void configure_system(int type, const string& param) {
    if (!is_root()) throw runtime_error("Failed to run! Are you root?");
    int confret;
    switch (type) {
        case 1:
            confret = system(("cp " + param + " /etc/fpm.json").c_str());
            if (confret != 0) throw runtime_error("Failed to copy " + param + " to /etc/fpm.json");
            cout << "Copied " << param << " to /etc/fpm.json" << endl;
            break;
        case 2:
            confret = system("mkdir -p /var/cache/fpm/");
            if (confret != 0) throw runtime_error("Failed to create directory /var/cache/fpm/");
            cout << "Setup fpm directory /var/cache/fpm/" << endl;
            break;
        case 3:
            if (!config::Experimental::usermode) throw runtime_error("This feature is only available in user mode!");
            confret = system(("mkdir -p /home/" + param + "/.fpm/").c_str());
            if (confret != 0) throw runtime_error("Failed to create directory /home/" + param + "/.fpm/");
            cout << "Setup fpm directory /home/" + param + "/.fpm/" << endl;
            break;
        default:
            throw invalid_argument("Invalid configuration type");
    }
}

void print_about() {
    const char* compileDate = __DATE__;
    const char* compileTime = __TIME__;
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%s %s", compileDate, compileTime);
    cout << "Version: " << version << endl;
    cout << "Build date: " << buffer << endl;
}

void install_package(const string& package) {
    string mirlink = get_mirlink();
    string command = "wget -q " + mirlink + package + "/adds";
    if (system(command.c_str()) != 0) throw runtime_error("Failed to acquire files for " + package);
    
    cout << "Binaries to download:" << endl;
    system("cat adds");
    
    ifstream addsfile("adds");
    if (!addsfile.is_open()) throw runtime_error("Could not open adds file!");
    string line;
    while (getline(addsfile, line)) {
        string download_command = "wget -q --show-progress " + mirlink + package + "/" + line;
        cout << "Downloading " << line << endl;
        if (system(download_command.c_str()) != 0) {
            throw runtime_error("Failed to download " + line);
        }
    }
    addsfile.close();
    
    ifstream versionfile("version");
    if (!versionfile.is_open()) throw runtime_error("Could not open version file!");
    string versioncontent;
    getline(versionfile, versioncontent);
    versionfile.close();
    
    cout << "Installed " << package << " version " << versioncontent << endl;
}

void uninstall_package(const string& package) {
    json fpmjson;
    ifstream fpmjsonfile("/etc/fpm.json");
    if (!fpmjsonfile.is_open()) throw runtime_error("Failed to open fpm.json!");
    fpmjsonfile >> fpmjson;
    fpmjsonfile.close();
    
    if (!fpmjson.contains("packages") || !fpmjson["packages"].is_array()) {
        throw runtime_error("Invalid fpm.json structure!");
    }
    
    auto& packlist = fpmjson["packages"];
    auto iter = remove(packlist.begin(), packlist.end(), package);
    if (iter != packlist.end()) {
        packlist.erase(iter, packlist.end());
        cout << "Uninstalled " << package << endl;
    } else {
        throw runtime_error("Package " + package + " not found!");
    }
    
    ofstream outputfpmjson("/etc/fpm.json");
    if (!outputfpmjson.is_open()) throw runtime_error("Could not open fpm.json for writing!");
    outputfpmjson << setw(4) << fpmjson << endl;
    outputfpmjson.close();
    cout << "Updated package list." << endl;
}
}
#endif // FPM_LIBRARY_HPP