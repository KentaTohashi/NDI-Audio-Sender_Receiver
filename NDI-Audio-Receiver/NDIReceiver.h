//
// Created by tohashi on 19/06/07.
//

#ifndef NDI_AUDIO_RECEIVER_NDIRECEIVER_H
#define NDI_AUDIO_RECEIVER_NDIRECEIVER_H

#include <Processing.NDI.Lib.h>
#include <iostream>
#include <thread>
#include <alsa/asoundlib.h>
#include <chrono>
#include <queue>
#include <mutex>
using namespace std;

class NDIReceiver {
public:
    virtual ~NDIReceiver();

public:
    NDIReceiver();

private:

    thread ndi_receive_thread;
    thread alsa_out_thread;
    mutex m;
    int buff_size;
    int frame;
    NDIlib_recv_instance_t pNDI_recv;
    NDIlib_recv_create_v3_t NDI_recv_create_desc;

    snd_pcm_t *pcm_handle;

    uint8_t *receive_buff;
    uint8_t *out_buff;
    queue<uint8_t> *buff_queue;
    void receive_function();
    void alsa_out_function();
};


#endif //NDI_AUDIO_RECEIVER_NDIRECEIVER_H
