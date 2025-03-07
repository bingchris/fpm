#include "fpm.hpp"
#include <iostream>
#include <stdexcept>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <in|out|configure|about> [args]" << endl;
        return 1;
    }
    try {
        string action = argv[1];
        if (action == "in" || action == "i") {
            if (argc < 3) throw invalid_argument("Missing package name for installation");
            fpm::install_package(argv[2]);
        } else if (action == "out" || action == "o") {
            if (argc < 3) throw invalid_argument("Missing package name for uninstallation");
            fpm::uninstall_package(argv[2]);
        } else if (action == "configure" || action == "c") {
            if (!fpm::is_root()) throw runtime_error("Failed to run! Are you root?");
            if (argc < 4) throw invalid_argument("Invalid configure command");
            string typeStr = argv[2];
            int type = (typeStr == "config") ? 1 : (typeStr == "dir") ? 2 : (typeStr == "diruser") ? 3 : 0;
            fpm::configure_system(type, argv[3]);
        } else if (action == "about") {
            fpm::print_about();
        } else if (action=="fasturl"){
            cout << fpm::findbestmirror() << endl;
        } else {
            cerr << "Invalid action: " << action << endl;
            return 1;
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}