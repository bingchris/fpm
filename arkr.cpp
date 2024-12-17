#include <iostream>
#include <stdio.h>
#include <stdexcept>
#include <array>
#include <memory>
using namespace std;
string get_mirlink() {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen("cat /etc/arkr/mirlink | tr -d '\n'", "r"), pclose); /*http://example.com/packages/arch/*/
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


int main(int argc, char *argv[]) {
    string setmirlink = get_mirlink();
    int action; /*0 = help, 1 = install, 2 = uninstall*/
    if (argv[1]=="in"){
        action=1;
    } else if (argv[1]=="out") {
        action=2;
    }
    for (int i=2; i < argc; ++i) {
        if (action=1) {
            unique_ptr<FILE, decltype(&pclose)> pipe(popen(("wget -q --show-progress " + setmirlink + argv[i]).c_str(), "r"), pclose);
        }
    }

}