//
// Created by tohashi on 19/06/07.
//

#ifndef NDI_AUDIO_SENDER_NDISENDER_H
#define NDI_AUDIO_SENDER_NDISENDER_H

#include <thread>
#include <alsa/asoundlib.h>
#include <iostream>
#include <Processing.NDI.Lib.h>
#include <queue>
#include <mutex>

#define DEFAULT_RATE 44100
#define DEFAULT_CHANNELS 1
#define DEFAULT_DEVICE "default"
using namespace std;

class NDISender {
public:
    NDISender();

    NDISender(unsigned int rate, int channels);

private:
    void send_function();

    void record_function();

    int channels;
    int rate;
    snd_pcm_t *pcm_handle;
    string pcm_device = DEFAULT_DEVICE;
    int buff_size;
    uint8_t *record_buff;
    uint8_t *send_buff;
    bool exit_flag;
    NDIlib_audio_frame_interleaved_16s_t NDI_audio_frame;
    int length;

    queue<uint8_t> *buff_queue;

    thread ndi_send_thread;
    thread alsa_record_thread;

    mutex m;
public:
    virtual ~NDISender();

private:
    /***
     * 送信プロパティ構造体
     */
    NDIlib_send_create_t NDI_send_create_desc;
    /***
    * 送信用インスタンス
    */
    NDIlib_send_instance_t m_pNDI_send;
    snd_pcm_uframes_t frames;
};


#endif //NDI_AUDIO_SENDER_NDISENDER_H
