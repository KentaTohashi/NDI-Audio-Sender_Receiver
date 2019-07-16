#include <iostream>
#include "NDIReceiver.h"
#include <csignal>
using namespace std;

int main(int argc, char *argv[]) {

    sigset_t ss;
    int signal_number;
    if(argc != 2){
        cerr << "第一引数にチャンネル番号を指定してください" << endl;
    }
    int ch = stol(argv[1], nullptr, 10);
    auto *receiver = new NDIReceiver(ch);
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
    delete receiver;


    return 0;

}