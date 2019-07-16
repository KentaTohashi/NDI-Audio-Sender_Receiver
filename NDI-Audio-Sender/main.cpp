#include <iostream>
#include <csignal>
using namespace std;

#include "NDISender.h"
int main(int argc, char *argv[]) {
    sigset_t ss;
    int signal_number;
    if(argc != 2){
        cerr << "第一引数にチャンネルを指定してください" << endl;
    }
    int ch = stol(argv[1], nullptr, 10);
    auto *sender = new NDISender(ch);
    bool end_flag = false;
    while (!end_flag) {
        if (sigwait(&ss, &signal_number) == 0) {
            switch (signal_number) {
                case SIGINT:
                case SIGHUP:
                    end_flag = true;
                    break;
                default:
                    break;
            }
        }
    }
    delete sender;


    return 0;
}