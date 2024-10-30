#include <iostream>
#include <string>
#include <curl/curl.h>
#include <fstream>

//save file to directory
//TODO: add an option to save to another directory (and make it so by default its the current user directory)
size_t save_package(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t writtento = fwrite(ptr, size, nmemb, stream);
    return writtento;
}
int main(int argc, char* argv[]) {
    CURL* curl;
    FILE* fp;
    CURLcode res;
    std::string action;
    int currentarg;
    currentarg = argc;
    for (int i = 1; i < argc; ++i) {
        currentarg=i;
        if (argv[i][0] == '-') {
            action = argv[i];
            if (action == "-download") {
                std::cout << "downloading idk" << std::endl;
            };
            const char* package_url = argv[currentarg+1];
            std::cout << package_url << std::endl;
            std::cout << "Action: " << action << ", its the " << currentarg << std::endl;
        } else {
            std::cout << argv[i] << std::endl;
        }
    }
    std::cout << currentarg << std::endl;
    //std::cout << "Action: " << action << std::endl;
    return 0;
}
