//
// Created by tohashi on 19/06/07.
//

#include "NDISender.h"

NDISender::NDISender(int ch) : NDISender(ch, DEFAULT_RATE, DEFAULT_CHANNELS) {

}

NDISender::NDISender(int ch, unsigned int rate, int channels) {
    pcm_handle = nullptr;
    frames = 1024;
    this->channels = channels;
    this->rate = rate;
    snd_pcm_hw_params_t *hw_params;
    int pcm;
    // デバイスを録音モードで開く
    if ((pcm = snd_pcm_open(&pcm_handle, pcm_device.c_str(),
                            SND_PCM_STREAM_CAPTURE, 0)) < 0)
        cerr << "ERROR: Can't open \"" << pcm_device << "\" PCM device." << snd_strerror(pcm) << endl;

    // デフォルトパラメータで初期化
    snd_pcm_hw_params_alloca(&hw_params);

    snd_pcm_hw_params_any(pcm_handle, hw_params);

    /* パラメータを設定 */
    if ((pcm = snd_pcm_hw_params_set_access(pcm_handle, hw_params,
                                            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        cerr << "ERROR: Can't set interleaved mode." << snd_strerror(pcm) << endl;

    if ((pcm = snd_pcm_hw_params_set_format(pcm_handle, hw_params,
                                            SND_PCM_FORMAT_S16_LE)) < 0)
        cerr << "ERROR: Can't set format." << snd_strerror(pcm) << endl;

    if ((pcm = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels)) < 0)
        cerr << "ERROR: Can't set channels number." << snd_strerror(pcm) << endl;

    if ((pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, nullptr)) < 0)
        cerr << "ERROR: Can't set rate." << snd_strerror(pcm) << endl;
    if ((pcm = snd_pcm_hw_params_set_period_size(pcm_handle, hw_params, frames, 0)) < 0)
        cerr << "ERROR: Can't set frame size." << snd_strerror(pcm) << endl;

    if ((pcm = snd_pcm_hw_params_set_periods(pcm_handle, hw_params, 16, 0)) < 0)
        cerr << "ERROR: Can't set period size." << snd_strerror(pcm) << endl;
    /* パラメータを書き込み */
    if ((pcm = snd_pcm_hw_params(pcm_handle, hw_params)) < 0)
        cerr << "ERROR: Can't set harware parameters. " << snd_strerror(pcm) << endl;

    snd_pcm_hw_params_get_period_size(hw_params, &frames, nullptr);

    buff_size = frames * channels * 2 /* 2 -> sample size */;

    record_buff = (uint8_t *) malloc(sizeof(uint8_t) * buff_size);
    send_buff = (uint8_t *) malloc(sizeof(uint8_t) * buff_size);
    string instance_name = "Audio" + to_string(ch);
    NDI_send_create_desc.p_ndi_name = instance_name.c_str(); // 送信インスタンス
    NDI_send_create_desc.clock_video = false; // 送信時間を同期させるかどうか
    NDI_send_create_desc.clock_audio = true; // 送信時間を同期させるかどうか
    m_pNDI_send = NDIlib_send_create(&NDI_send_create_desc); // NDI送信インスタンス生成
    if (m_pNDI_send == nullptr) {
        cerr << "cannot create NDI sender instance" << endl;
        throw runtime_error("cannot create NDI sender instance");
    }

    NDI_audio_frame.sample_rate = rate;
    NDI_audio_frame.no_channels = channels;
    NDI_audio_frame.no_samples = frames;
    length = frames * channels * sizeof(short);
    NDI_audio_frame.p_data = (short *) malloc(length);
    buff_queue = new queue<uint8_t>();
    exit_flag = false;
    ndi_send_thread = thread(&NDISender::send_function, this);
    alsa_record_thread = thread(&NDISender::record_function, this);
}

void NDISender::send_function() {

    while (!exit_flag) {
        m.lock();
        if (buff_queue->size() > buff_size) {
            for (int i = 0; i < buff_size; i++) {
                send_buff[i] = buff_queue->front();
                buff_queue->pop();
            }
            m.unlock();
            memcpy(NDI_audio_frame.p_data, send_buff, buff_size);
            NDIlib_util_send_send_audio_interleaved_16s(m_pNDI_send, &NDI_audio_frame);
        } else {
            m.unlock();
            usleep(100);
        }

    }
}

NDISender::~NDISender() {
    exit_flag = true;
    ndi_send_thread.join();
    free(NDI_audio_frame.p_data);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    free(record_buff);
    free(send_buff);
    NDIlib_send_destroy(m_pNDI_send);
}

void NDISender::record_function() {
    int ret;

    while (true) {
        if ((ret = snd_pcm_readi(pcm_handle, record_buff, frames)) == -EPIPE) {
            cerr << "XRUN." << endl;
            snd_pcm_prepare(pcm_handle);
        } else if (ret < 0) {
            string exception_str = "ERROR. Can't write to PCM device. ";
            exception_str += string(snd_strerror(ret));
            throw runtime_error(exception_str);
        }
        m.lock();
        for (int i = 0; i < buff_size; i++) {
            buff_queue->push(record_buff[i]);
        }
        m.unlock();
    }
}


