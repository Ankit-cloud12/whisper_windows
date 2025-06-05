#ifndef WHISPERSTUB_H
#define WHISPERSTUB_H

// Stub definitions for Whisper types when building without whisper.cpp
// This allows the application to compile and run without the actual Whisper library

#include <string>

namespace whisper {

// Stub whisper context
struct whisper_context {
    void* dummy;
};

// Stub whisper parameters
struct whisper_full_params {
    int n_threads = 4;
    int n_processors = 1;
    int offset_ms = 0;
    int duration_ms = 0;
    bool translate = false;
    bool no_context = true;
    bool single_segment = false;
    bool print_special = false;
    bool print_progress = false;
    bool print_realtime = false;
    bool print_timestamps = true;
    std::string language = "en";
    bool detect_language = false;
    int max_tokens = 0;
    float temperature = 0.0f;
    float temperature_inc = 0.2f;
    float entropy_thold = 2.4f;
    float logprob_thold = -1.0f;
    float no_speech_thold = 0.6f;
};

// Stub functions
inline whisper_context* whisper_init_from_file(const char*) { return nullptr; }
inline void whisper_free(whisper_context*) {}
inline int whisper_full(whisper_context*, whisper_full_params, const float*, int) { return -1; }
inline whisper_full_params whisper_full_default_params(int) { return whisper_full_params{}; }
inline int whisper_full_n_segments(whisper_context*) { return 0; }
inline const char* whisper_full_get_segment_text(whisper_context*, int) { return ""; }
inline int64_t whisper_full_get_segment_t0(whisper_context*, int) { return 0; }
inline int64_t whisper_full_get_segment_t1(whisper_context*, int) { return 0; }
inline const char* whisper_lang_str(int) { return "en"; }
inline int whisper_lang_auto_detect(whisper_context*, int, int, float*) { return 0; }

// Sampling strategies
enum whisper_sampling_strategy {
    WHISPER_SAMPLING_GREEDY,
    WHISPER_SAMPLING_BEAM_SEARCH,
};

}  // namespace whisper

#endif // WHISPERSTUB_H