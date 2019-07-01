#include <iostream>
#include <csignal>
using namespace std;

#include "NDISender.h"
int main() {
    sigset_t ss;
    int signal_number;
    auto *sender = new NDISender();
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