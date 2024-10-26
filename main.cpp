#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string action;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            action = argv[i][1];
            std::cout << "Action: " << action << std::endl;
        } else {
            std::cout << argv[i] << std::endl;
        }
    }

    //std::cout << "Action: " << action << std::endl;
    return 0;
}
