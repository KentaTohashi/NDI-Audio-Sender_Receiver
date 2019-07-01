#include <iostream>
#include "NDIReceiver.h"
#include <csignal>
using namespace std;

int main() {

    sigset_t ss;
    int signal_number;
    auto *receiver = new NDIReceiver();
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