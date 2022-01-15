#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include <portaudio.h>
#include <pthread.h>

#ifndef M_PI
    #define M_PI 3.14
#endif

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

typedef struct playing_key {
    uint64_t last_active_timestamp;
    uint64_t timestamp;
    bool     active, open;
    double   base_freq;
} key;

struct callback_data {
    uint64_t progress;
    key      key[10];
} callback_data;

const int sustain_duration = 5000;

int audio_callback(const void *input, void *output,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData) {
    float *out = (float*)output;

    for(unsigned long i=0; i<framesPerBuffer; i++) {
        //out[i] = sin((float)i/framesPerBuffer);
        out[i] = 0;

        int progress = callback_data.progress + i;
        for(int j=0; j<10; j++) {
            key* key = &callback_data.key[j];
            if(key->open == false)
                continue;

            float s = sin(M_PI*2.0*progress*key->base_freq/44100.0);
            float a = 0.5;
            if(key->active == false) {
                a = (float)(sustain_duration-progress+key->last_active_timestamp)/sustain_duration;

                if(a == 0.0)
                    key->open = false;
            }
            out[i] += s * a;
        }

        out[i] = MIN(out[i], 1.0);
    }

    callback_data.progress += framesPerBuffer;

    return paContinue;
}

PaStream *stream;

void audio_init() {
    PaStreamParameters outputParameters;

    Pa_Initialize();

    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    Pa_OpenStream(
            &stream,
            NULL,
            &outputParameters,
            44100,
            64,
            paClipOff,
            audio_callback,
            NULL); // callback argument

    Pa_StartStream(stream);
}

// https://pages.mtu.edu/~suits/NoteFreqCalcs.html
// https://pages.mtu.edu/~suits/notefreqs.html
float audio_key_to_freq(char key) {
    const char notekeys[] = {
        'q', '2', 'w', '3', 'e',
        'r', '5', 't', '6', 'y', '7', 'u',
        'i', '9', 'o', '0', 'p',
        '<', 'a', 'z', 's', 'x', 'd', 'c',
        'v', 'g', 'b', 'h', 'n',
        'm', 'k', ',', 'l', '.', ';', '/'
    };

    int found = -1;
    for(int i=0; i<sizeof(notekeys)/sizeof(char); i++) {
        if(key == notekeys[i]) {
            found = i;
            break;
        }
    }

    if(found == -1)
        return 0.0;

    return 261.43 * pow(pow(2.0, 1.0/12.0), found);
}

void audio_key_play(char key) {
    // we don't need to check for already existing frequency because you cant
    // press a key twice before releasing it
    float freq = audio_key_to_freq(key);

    for(int i=0; i<10; i++) {
        if(callback_data.key[i].open == true &&
           callback_data.key[i].base_freq != freq)
            continue;

        callback_data.key[i].open = true;
        callback_data.key[i].active = true;
        callback_data.key[i].timestamp = callback_data.progress;
        callback_data.key[i].base_freq = freq;
        break;
    }
}

void audio_key_stop(char key) {
    // do some magic to convert key to frequency
    double target_freq = audio_key_to_freq(key);
    for(int i=0; i<10; i++) {
        if(callback_data.key[i].base_freq == target_freq) {
            callback_data.key[i].active = false;
            callback_data.key[i].last_active_timestamp = callback_data.progress;
        }
    }
}

void audio_clean() {
    Pa_StopStream(stream);
    Pa_CloseStream(stream);

    Pa_Terminate();
}

