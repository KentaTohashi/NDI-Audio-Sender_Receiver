//
// Created by tohashi on 19/06/07.
//

#include <cstring>
#include "NDIReceiver.h"

NDIReceiver::NDIReceiver() {
    //メンバ変数初期化
    pcm_handle = nullptr;
    buff_queue = new queue<uint8_t>();
    receive_buff = nullptr;
    out_buff = nullptr;

    NDIlib_find_create_t NDI_find_create_desc; // 検索に必要な構造体定義(デフォルト値)
    NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2(&NDI_find_create_desc); // 検索インスタンス生成
    bool exit_find_loop = false;
    const NDIlib_source_t *p_sources;
    uint32_t no_sources;
    string str_findname;
    // 生成チェック
    if (!pNDI_find) {
        throw runtime_error("NDIlib_find_instance_t create failure.");

    }
    cout << "SEARCHING..." << endl;
    while (!exit_find_loop) {
        NDIlib_find_wait_for_sources(pNDI_find, 1000); // 見つかるまで待機（タイムアウト1000msec）
        p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources); // リソース情報の取得
        for (int i = 0; i < (int) no_sources; i++) {
            str_findname = string(p_sources[i].p_ndi_name); // 発見したリソース名の格納

            if (str_findname.find("My 16bpp Audio") != -1) {
                cout << "HIT" << endl;
                this->NDI_recv_create_desc.source_to_connect_to = p_sources[i];
                exit_find_loop = true; // ループ終了フラグオン
                break;
            }
        }
    }


    NDI_recv_create_desc.p_ndi_recv_name = "Example Audio Converter Receiver";

// Create the receiver
    pNDI_recv = NDIlib_recv_create_v3(&NDI_recv_create_desc);
    if (!pNDI_recv) {
        return;
    }
    ndi_receive_thread = thread(&NDIReceiver::receive_function, this);
    NDIlib_find_destroy(pNDI_find); // ファインダの削除
}

void NDIReceiver::receive_function() {
    cout << "thread" << endl;

    NDIlib_audio_frame_v2_t audio_frame;
    snd_pcm_hw_params_t *hw_params;

    bool init = false;
    int ret;
    int channels = 0, length = 0;
    unsigned rate;
    if (!pNDI_recv) {
        throw runtime_error("cannot careate NDI instance");
    }
    while (true) {
        ret = NDIlib_recv_capture_v2(pNDI_recv, nullptr, &audio_frame, nullptr, 1000);
        if (ret == NDIlib_frame_type_audio) {
            if (!init) {
                cout << "init" << endl;
                if ((ret = snd_pcm_open(&pcm_handle, "default",
                                        SND_PCM_STREAM_PLAYBACK, 0)) < 0)
                    printf("ERROR: Can't open \"%s\" PCM device. %s\n",
                           "default", snd_strerror(ret));
                frame = audio_frame.no_samples;
                channels = audio_frame.no_channels;
                length = frame * channels * sizeof(uint8_t) * 2;
                rate = audio_frame.sample_rate;
                receive_buff = (uint8_t *) malloc(length);
                // デフォルトパラメータで初期化
                snd_pcm_hw_params_alloca(&hw_params);

                snd_pcm_hw_params_any(pcm_handle, hw_params);

                // パラメータを設定
                if ((ret = snd_pcm_hw_params_set_access(pcm_handle, hw_params,
                                                        SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
                    throw runtime_error(string("ERROR: Can't set interleaved mode.") + snd_strerror(ret));

                if ((ret = snd_pcm_hw_params_set_format(pcm_handle, hw_params,
                                                        SND_PCM_FORMAT_S16_LE)) < 0)
                    cerr << "ERROR: Can't set format." << snd_strerror(ret) << endl;

                if ((ret = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels)) < 0)
                    cerr << "ERROR: Can't set channels number." << snd_strerror(ret) << endl;

                if ((ret = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, nullptr)) < 0)
                    cerr << "ERROR: Can't set rate." << snd_strerror(ret) << endl;
                if ((ret = snd_pcm_hw_params_set_period_size(pcm_handle, hw_params, frame, 0)) < 0)
                    cerr << "ERROR: Can't set frame size" << snd_strerror(ret) << endl;
                if ((ret = snd_pcm_hw_params_set_periods(pcm_handle, hw_params, 8, 0)) < 0)
                    cerr << "ERROR: Can't set period size." << snd_strerror(ret) << endl;
                // パラメータを書き込み
                if ((ret = snd_pcm_hw_params(pcm_handle, hw_params)) < 0)
                    cerr << "ERROR: Can't set harware parameters. " << snd_strerror(ret) << endl;

                buff_size = frame * channels * 2;

                out_buff = (uint8_t *) malloc(sizeof(uint8_t) * buff_size);

            }
            // Allocate enough space for 16bpp interleaved buffer
            NDIlib_audio_frame_interleaved_16s_t audio_frame_16bpp_interleaved;
            audio_frame_16bpp_interleaved.reference_level = 20;    // We are going to have 20dB of headroom
            audio_frame_16bpp_interleaved.p_data = new short[audio_frame.no_samples * audio_frame.no_channels];
            NDIlib_util_audio_to_interleaved_16s_v2(&audio_frame, &audio_frame_16bpp_interleaved);

            NDIlib_recv_free_audio_v2(pNDI_recv, &audio_frame);
            memcpy(receive_buff, audio_frame_16bpp_interleaved.p_data, buff_size);

            m.lock();
            for (int i = 0; i < buff_size; i++) {
                buff_queue->push(receive_buff[i]);
            }
            m.unlock();
            if (!init) {

                alsa_out_thread = thread(&NDIReceiver::alsa_out_function, this);
                init = true;
            }
        }
    }

}


NDIReceiver::~NDIReceiver() {
    if (receive_buff != nullptr) {
        free(receive_buff);
    }
    if (out_buff != nullptr) {
        free(out_buff);
    }
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    NDIlib_recv_destroy(pNDI_recv);
}

void NDIReceiver::alsa_out_function() {
    int ret;
    int queue_size = 0;
    do {
        queue_size = buff_queue->size();
        usleep(1000);
    } while (queue_size < buff_size * 5);
    while (true) {
        if (buff_queue->size() > buff_size) {

            m.lock();
            for (int i = 0; i < buff_size; i++) {
                out_buff[i] = buff_queue->front();
                buff_queue->pop();
            }
            m.unlock();
            if ((ret = snd_pcm_wait(pcm_handle, 1000)) < 0) {
                fprintf(stderr, "poll failed (%s)\n", snd_strerror(ret));
                break;
            }

            if ((ret = snd_pcm_writei(pcm_handle, out_buff, frame)) == -EPIPE) {
                //snd_pcm_prepare(pcm_handle);
                snd_pcm_recover(pcm_handle, ret, 0);
                m.lock();
                queue_size = buff_queue->size();
                m.unlock();
                cout << queue_size << endl;
                do {
                    m.lock();
                    queue_size = buff_queue->size();
                    m.unlock();
                    usleep(1000);
                } while (queue_size < buff_size * 5);
            } else if (ret < 0) {
                throw runtime_error(string("ERROR. Can't write to PCM device.") + snd_strerror(ret));
            }
        } else {
            usleep(10);
        }
    }

}
