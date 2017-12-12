#include <stdio.h>
#include <string.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/pulseaudio.h>

#define BUFSIZE 1024

pa_mainloop* m_pulseaudio_mainloop;

struct audio_data {
    char* source;
    int terminate;
};

void cb(__attribute__((unused)) pa_context* pulseaudio_context, const pa_server_info* i, void* userdata) {
    // Get default sink name
    struct audio_data* audio = (struct audio_data*)userdata;
    audio->source = malloc(sizeof(char) * 1024);
    strcpy(audio->source, i->default_sink_name);

    // Append .monitor suffix
    audio->source = strcat(audio->source, ".monitor");

    // Quit mainloop
    pa_context_disconnect(pulseaudio_context);
    pa_context_unref(pulseaudio_context);
    pa_mainloop_quit(m_pulseaudio_mainloop, 0);
    pa_mainloop_free(m_pulseaudio_mainloop);
}

void pulseaudio_context_state_callback(pa_context* pulseaudio_context, void* userdata) {
    switch (pa_context_get_state(pulseaudio_context)) {
        case PA_CONTEXT_UNCONNECTED:
            break;
        case PA_CONTEXT_CONNECTING:
            break;
        case PA_CONTEXT_AUTHORIZING:
            break;
        case PA_CONTEXT_SETTING_NAME:
            break;
        case PA_CONTEXT_READY:
            pa_operation_unref(
                pa_context_get_server_info(pulseaudio_context, cb, userdata)
            );
            break;
        case PA_CONTEXT_FAILED:
            printf("Failed to connect to PulseAudio server.\n");
            break;
        case PA_CONTEXT_TERMINATED:
            printf("Terminated.\n");
            pa_mainloop_quit(m_pulseaudio_mainloop, 0);
            break;
    }
}

void get_default_sink(void* data) {
    struct audio_data* audio = (struct audio_data*)data;
    pa_mainloop_api* mainloop_api;
    pa_context* pulseaudio_context;
    int ret;

    // Create a mainloop API
    m_pulseaudio_mainloop = pa_mainloop_new();

    mainloop_api = pa_mainloop_get_api(m_pulseaudio_mainloop);
    pulseaudio_context = pa_context_new(mainloop_api, "test device list");

    // Connect to the server
    pa_context_connect(pulseaudio_context, NULL, PA_CONTEXT_NOFLAGS, NULL);

    // Set up state callback
    pa_context_set_state_callback(pulseaudio_context, pulseaudio_context_state_callback, (void*)audio);

    if (!(ret = pa_mainloop_iterate(m_pulseaudio_mainloop, 0, &ret))) {
        printf("Could not open PulseAudio mainloop to get default device: %d\n"
               "Make sure PulseAudio is running.\n",
               ret);

        exit(EXIT_FAILURE);
    }

    pa_mainloop_run(m_pulseaudio_mainloop, &ret);
}

int main(int argc, char* argv[]) {
    struct audio_data *audio = malloc(sizeof(struct audio_data));

    get_default_sink(audio);

    int i, n;
    int16_t buf[BUFSIZE / 2];

    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };

    static const pa_buffer_attr pb = {
        .maxlength = BUFSIZE * 2,
        .fragsize = BUFSIZE
    };

    pa_simple* s = NULL;
    int error;

    if (!(s = pa_simple_new(NULL, "test", PA_STREAM_RECORD, audio->source, "audio for test", &ss, NULL, &pb, &error))) {
       fprintf(stderr, __FILE__": Could not open PulseAudio surce: %s, %s.", audio->source, pa_strerror(error));
       exit(EXIT_FAILURE);
    }

    n = 0;

    for (;;) {
        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s.", pa_strerror(error));
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < BUFSIZE / 2; i += 2) {
            printf("%d : %d\n", buf[i], buf[i + 1]);

            n++;
            if (n == 2048 - 1) n = 0;
        }

        if (audio->terminate == 1) {
            pa_simple_free(s);
            break;
        }
    }

    return 0;
}