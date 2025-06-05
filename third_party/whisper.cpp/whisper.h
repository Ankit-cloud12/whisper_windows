#ifndef WHISPER_H
#define WHISPER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef WHISPER_SHARED
#    ifdef _WIN32
#        ifdef WHISPER_BUILD
#            define WHISPER_API __declspec(dllexport)
#        else
#            define WHISPER_API __declspec(dllimport)
#        endif
#    else
#        define WHISPER_API __attribute__ ((visibility ("default")))
#    endif
#else
#    define WHISPER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

    //
    // C interface
    //
    // The following interface is thread-safe as long as the sample whisper_context is not used by multiple threads
    // simultaneously.
    //
    // Basic usage:
    //
    //     #include "whisper.h"
    //
    //     ...
    //
    //     whisper_context * ctx = whisper_init_from_file("path/to/ggml-base.en.bin");
    //
    //     if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
    //         fprintf(stderr, "failed to process audio\n");
    //         return 7;
    //     }
    //
    //     const int n_segments = whisper_full_n_segments(ctx);
    //     for (int i = 0; i < n_segments; ++i) {
    //         const char * text = whisper_full_get_segment_text(ctx, i);
    //         printf("%s", text);
    //     }
    //
    //     whisper_free(ctx);
    //
    //     ...
    //

    struct whisper_context;
    struct whisper_state;

    typedef int32_t whisper_token;
    typedef struct whisper_context whisper_context;
    typedef struct whisper_state   whisper_state;

    typedef struct whisper_token_data {
        whisper_token id;  // token id
        whisper_token tid; // forced timestamp token id

        float p;           // probability of the token
        float plog;        // log probability of the token
        float pt;          // probability of the timestamp token
        float ptsum;       // sum of probabilities of all timestamp tokens

        // token-level timestamp data
        // do not use if you haven't computed token-level timestamps
        int64_t t0;        // start time of the token
        int64_t t1;        // end   time of the token

        float vlen;        // voice length of the token
    } whisper_token_data;

    typedef struct whisper_model_loader {
        void * context;

        size_t (*read)(void * ctx, void * output, size_t read_size);
        bool   (*eof)(void * ctx);
        void   (*close)(void * ctx);
    } whisper_model_loader;

    // Various functions for loading a ggml whisper model.
    // Allocate (almost) all memory needed for the model.
    // Return NULL on failure
    WHISPER_API struct whisper_context * whisper_init_from_file(const char * path_model);
    WHISPER_API struct whisper_context * whisper_init_from_buffer(void * buffer, size_t buffer_size);
    WHISPER_API struct whisper_context * whisper_init_with_params(const char * path_model, struct whisper_context_params params);
    WHISPER_API struct whisper_context * whisper_init_with_params_no_state(const char * path_model, struct whisper_context_params params);

    // These multimodal models don't support loading from buffer.
    WHISPER_API struct whisper_context * whisper_init_from_file_with_params(const char * path_model, struct whisper_context_params params);
    WHISPER_API struct whisper_context * whisper_init_from_buffer_with_params(void * buffer, size_t buffer_size, struct whisper_context_params params);

    WHISPER_API struct whisper_context * whisper_init_from_loader_with_params(struct whisper_model_loader * loader, struct whisper_context_params params);

    WHISPER_API struct whisper_context_params whisper_context_default_params(void);

    // Frees all allocated memory
    WHISPER_API void whisper_free      (struct whisper_context * ctx);
    WHISPER_API void whisper_free_state(struct whisper_state * state);

    // Convert RAW PCM audio to log mel spectrogram.
    // The resulting spectrogram is stored inside the default state of the provided whisper context.
    // Returns 0 on success
    WHISPER_API int whisper_pcm_to_mel(
            struct whisper_context * ctx,
                    const float * samples,
                            int   n_samples,
                            int   n_threads);

    WHISPER_API int whisper_pcm_to_mel_with_state(
            struct whisper_context * ctx,
              struct whisper_state * state,
                    const float * samples,
                            int   n_samples,
                            int   n_threads);

    // Convert RAW PCM audio to log mel spectrogram but applies a Phase Vocoder to speed up the audio x2.
    // The resulting spectrogram is stored inside the default state of the provided whisper context.
    // Returns 0 on success
    WHISPER_API int whisper_pcm_to_mel_phase_vocoder(
        struct whisper_context * ctx,
                const float * samples,
                        int   n_samples,
                        int   n_threads);

    WHISPER_API int whisper_pcm_to_mel_phase_vocoder_with_state(
        struct whisper_context * ctx,
          struct whisper_state * state,
                const float * samples,
                        int   n_samples,
                        int   n_threads);

    // This can be used to set a custom log mel spectrogram inside the default state of the provided whisper context.
    // Use this instead of whisper_pcm_to_mel() if you want to provide your own log mel spectrogram.
    // n_mel must be 80
    // Returns 0 on success
    WHISPER_API int whisper_set_mel(
            struct whisper_context * ctx,
                    const float * data,
                            int   n_len,
                            int   n_mel);

    WHISPER_API int whisper_set_mel_with_state(
            struct whisper_context * ctx,
              struct whisper_state * state,
                    const float * data,
                            int   n_len,
                            int   n_mel);

    // Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text
    // Not thread safe for same context
    // Uses the specified decoding strategy to obtain the text.
    WHISPER_API int whisper_full(
                struct whisper_context * ctx,
            struct whisper_full_params   params,
                      const float * samples,
                              int   n_samples);

    WHISPER_API int whisper_full_with_state(
                struct whisper_context * ctx,
                  struct whisper_state * state,
            struct whisper_full_params   params,
                      const float * samples,
                              int   n_samples);

    // Split the input audio in chunks and process each chunk separately using whisper_full_with_state()
    // Result is stored in the default state of the context
    // Not thread safe if executed in parallel on the same context.
    // It seems this approach can offer some speedup in some cases.
    // However, the transcription accuracy can be worse at the beginning and end of each chunk.
    WHISPER_API int whisper_full_parallel(
                struct whisper_context * ctx,
            struct whisper_full_params   params,
                      const float * samples,
                              int   n_samples,
                              int   n_processors);

    // Number of generated text segments.
    // A segment can be a few words, a sentence, or even a paragraph.
    WHISPER_API int whisper_full_n_segments           (struct whisper_context * ctx);
    WHISPER_API int whisper_full_n_segments_from_state(struct whisper_state * state);

    // Language id associated with the context's default state
    WHISPER_API int whisper_full_lang_id(struct whisper_context * ctx);
    WHISPER_API int whisper_full_lang_id_from_state(struct whisper_state * state);

    // Get the start and end time of the specified segment.
    WHISPER_API int64_t whisper_full_get_segment_t0           (struct whisper_context * ctx, int i_segment);
    WHISPER_API int64_t whisper_full_get_segment_t0_from_state(struct whisper_state * state, int i_segment);

    WHISPER_API int64_t whisper_full_get_segment_t1           (struct whisper_context * ctx, int i_segment);
    WHISPER_API int64_t whisper_full_get_segment_t1_from_state(struct whisper_state * state, int i_segment);

    // Get whether the next segment is predicted as a speaker turn.
    WHISPER_API bool whisper_full_get_segment_speaker_turn_next           (struct whisper_context * ctx, int i_segment);
    WHISPER_API bool whisper_full_get_segment_speaker_turn_next_from_state(struct whisper_state * state, int i_segment);

    // Get the text of the specified segment.
    WHISPER_API const char * whisper_full_get_segment_text           (struct whisper_context * ctx, int i_segment);
    WHISPER_API const char * whisper_full_get_segment_text_from_state(struct whisper_state * state, int i_segment);

    // Get number of tokens in the specified segment.
    WHISPER_API int whisper_full_n_tokens           (struct whisper_context * ctx, int i_segment);
    WHISPER_API int whisper_full_n_tokens_from_state(struct whisper_state * state, int i_segment);

    // Get the token text of the specified token in the specified segment.
    WHISPER_API const char * whisper_full_get_token_text           (struct whisper_context * ctx, int i_segment, int i_token);
    WHISPER_API const char * whisper_full_get_token_text_from_state(struct whisper_context * ctx, struct whisper_state * state, int i_segment, int i_token);

    WHISPER_API whisper_token whisper_full_get_token_id           (struct whisper_context * ctx, int i_segment, int i_token);
    WHISPER_API whisper_token whisper_full_get_token_id_from_state(struct whisper_state * state, int i_segment, int i_token);

    // Get token data for the specified token in the specified segment.
    // This contains probabilities, timestamps, etc.
    WHISPER_API whisper_token_data whisper_full_get_token_data           (struct whisper_context * ctx, int i_segment, int i_token);
    WHISPER_API whisper_token_data whisper_full_get_token_data_from_state(struct whisper_state * state, int i_segment, int i_token);

    // Get the probability of the specified token in the specified segment.
    WHISPER_API float whisper_full_get_token_p           (struct whisper_context * ctx, int i_segment, int i_token);
    WHISPER_API float whisper_full_get_token_p_from_state(struct whisper_state * state, int i_segment, int i_token);

    ////////////////////////////////////////////////////////////////////////////

    // Temporary helpers needed for exposing ggml interface
    WHISPER_API int whisper_bench_memcpy     (int n_threads);
    WHISPER_API const char * whisper_bench_memcpy_str     (int n_threads);
    WHISPER_API int whisper_bench_ggml_mul_mat(int n_threads);
    WHISPER_API const char * whisper_bench_ggml_mul_mat_str(int n_threads);

    // Control logging output; default behavior is to print to stderr
    typedef void (*whisper_log_callback)(enum ggml_log_level level, const char * text, void * user_data);

    WHISPER_API void whisper_log_set(whisper_log_callback log_callback, void * user_data);

    // Performance information
    WHISPER_API void whisper_print_timings(struct whisper_context * ctx);
    WHISPER_API void whisper_reset_timings(struct whisper_context * ctx);

    // Print system information
    WHISPER_API const char * whisper_print_system_info(void);

    ////////////////////////////////////////////////////////////////////////////

    // Available sampling strategies
    enum whisper_sampling_strategy {
        WHISPER_SAMPLING_GREEDY,      // similar to OpenAI's GreedyDecoder
        WHISPER_SAMPLING_BEAM_SEARCH, // similar to OpenAI's BeamSearchDecoder
    };

    // Text segment callback
    // Called on every newly generated text segment
    // Use the whisper_full_...() functions to obtain the text and timestamp values
    // Params:
    //   - struct whisper_context * ctx
    //   - struct whisper_state * state // same as whisper_ctx_get_state(ctx)
    //   - int n_new // number of newly generated text segments
    //   - void * user_data
    typedef void (*whisper_new_segment_callback)(struct whisper_context * ctx, struct whisper_state * state, int n_new, void * user_data);

    // Progress callback
    typedef void (*whisper_progress_callback)(struct whisper_context * ctx, struct whisper_state * state, int progress, void * user_data);

    // Encoder begin callback
    // If not NULL, called before the encoder starts
    // If it returns false, the computation is aborted
    typedef bool (*whisper_encoder_begin_callback)(struct whisper_context * ctx, struct whisper_state * state, void * user_data);

    // Abort callback
    // If not NULL, called before ggml computation
    // If it returns true, the computation is aborted
    typedef bool (*whisper_abort_callback)(void * user_data);

    // Logits filter callback
    // Can be used to modify the logits before sampling
    // If not NULL, called after applying temperature to logits
    typedef void (*whisper_logits_filter_callback)(
            struct whisper_context * ctx,
              struct whisper_state * state,
                  const whisper_token_data * tokens,
                                       int   n_tokens,
                                     float * logits,
                                      void * user_data);

    // Parameters for the whisper_full() function
    // If you change the order or add new parameters, make sure to update the default values in whisper.cpp:
    // whisper_full_default_params()
    struct whisper_full_params {
        enum whisper_sampling_strategy strategy;

        int n_threads;
        int n_max_text_ctx;     // max tokens to use from past text as prompt for the decoder
        int offset_ms;          // start offset in ms
        int duration_ms;        // audio duration to process in ms

        bool translate;
        bool no_context;        // do not use past transcription (if any) as initial prompt for the decoder
        bool no_timestamps;     // do not generate timestamps
        bool single_segment;    // force single segment output (useful for streaming)
        bool print_special;     // print special tokens (e.g. <SOT>,  , <BEG>, etc.)
        bool print_progress;    // print progress information
        bool print_realtime;    // print results from within whisper.cpp (avoid it, use callback instead)
        bool print_timestamps;  // print timestamps for each text segment when printing realtime

        // [EXPERIMENTAL] token-level timestamps
        bool  token_timestamps; // enable token-level timestamps
        float thold_pt;         // timestamp token probability threshold (~0.01)
        float thold_ptsum;      // timestamp token sum probability threshold (~0.01)
        int   max_len;          // max segment length in characters
        bool  split_on_word;    // split on word rather than on token (when used with max_len)
        int   max_tokens;       // max tokens per segment (0 = no limit)

        // [EXPERIMENTAL] speed-up techniques
        // note: these can significantly reduce the quality of the output
        bool speed_up;          // speed-up the audio by 2x using Phase Vocoder
        bool debug_mode;        // enable debug_mode provides extra info (eg. dump log_mel)
        int  audio_ctx;         // overwrite the audio context size (0 = use default)

        // [EXPERIMENTAL] [TDRZ] tinydiarize
        bool tdrz_enable;       // enable tinydiarize speaker turn detection

        // A regular expression that matches tokens to suppress
        // The text will be normalized before matching against this regex, and any matches will be replaced with empty strings
        const char * suppress_regex;

        // tokens to provide to the whisper decoder as initial prompt
        // these are prepended to any existing text context from a previous call
        const char * initial_prompt;
        const whisper_token * prompt_tokens;
        int prompt_n_tokens;

        // for auto-detection, set to nullptr, "" or "auto"
        const char * language;
        bool detect_language;

        // common decoding parameters:
        bool suppress_blank;    // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L89
        bool suppress_non_speech_tokens; // ref: https://github.com/openai/whisper/blob/7858aa9c08d98f75575035ecd6481f462d66ca27/whisper/tokenizer.py#L224-L253

        float temperature;      // initial decoding temperature, ref: https://ai.stackexchange.com/a/32478
        float max_initial_ts;   // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
        float length_penalty;   // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L267

        // fallback parameters
        // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L274-L278
        float temperature_inc;
        float entropy_thold;    // similar to OpenAI's "compression_ratio_threshold"
        float logprob_thold;
        float no_speech_thold;  // TODO: not implemented

        struct {
            int best_of;        // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
        } greedy;

        struct {
            int beam_size;      // ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265
            float patience;     // TODO: not implemented, ref: https://arxiv.org/pdf/1508.04025.pdf
        } beam_search;

        // called for every newly generated text segment
        whisper_new_segment_callback new_segment_callback;
        void * new_segment_callback_user_data;

        // called on each progress update
        whisper_progress_callback progress_callback;
        void * progress_callback_user_data;

        // called each time before the encoder starts
        whisper_encoder_begin_callback encoder_begin_callback;
        void * encoder_begin_callback_user_data;

        // called by each decoder to filter the logits before sampling
        whisper_logits_filter_callback logits_filter_callback;
        void * logits_filter_callback_user_data;
    };

    WHISPER_API struct whisper_context_params {
        bool use_gpu;    // attempt to use GPU
        int  gpu_device; // device to use for GPU compute (CUDA, OpenCL, etc.)

        // [EXPERIMENTAL] Token-level timestamps with DTW
        bool dtw_token_timestamps;
        enum whisper_alignment_heads_preset {
            WHISPER_AHEADS_NONE,
            WHISPER_AHEADS_N_TOP_MOST, // All heads from the top-most n_text_layer layers
            // TODO: Add more presets for specific datasets.
        } dtw_aheads_preset;
        int dtw_n_top; // number of top scoring alignment heads to average (only used when preset is WHISPER_AHEADS_N_TOP_MOST)
        const char * dtw_aheads_path; // path to a file containing the alignment head indices (overrides preset)
        size_t dtw_mem_size; // [EXPERIMENTAL] maximum size in bytes for the cross-attention heads alignment memory pool (0 = default)
    };

    // NOTE: this function allocates memory, and it is the responsibility of the caller to free the pointer - see whisper_free_context_params & whisper_free_params()
    WHISPER_API struct whisper_full_params * whisper_full_default_params_by_ref(enum whisper_sampling_strategy strategy);
    WHISPER_API struct whisper_full_params   whisper_full_default_params(enum whisper_sampling_strategy strategy);

    // NOTE: this function allocates memory, and it is the responsibility of the caller to free the pointer - see whisper_free_context_params & whisper_free_params()
    WHISPER_API struct whisper_context_params * whisper_context_default_params_by_ref(void);

    WHISPER_API void whisper_free_context_params(struct whisper_context_params * params);
    WHISPER_API void whisper_free_params(struct whisper_full_params * params);

    // Return the id of the specified language, returns -1 if not found
    // Examples:
    //   "de" -> 2
    //   "german" -> 2
    WHISPER_API int whisper_lang_id(const char * lang);

    // Return the short string of the specified language id (e.g. 2 -> "de"), returns nullptr if not found
    WHISPER_API const char * whisper_lang_str(int id);

    // Use mel data at offset_ms to try and auto-detect the spoken language
    // Make sure to call whisper_pcm_to_mel() or whisper_set_mel() first
    // Returns the top language id or negative on failure
    // If not null, fills the lang_probs array with the probabilities of all languages
    // The array must be whisper_lang_max_id() + 1 in size
    // ref: https://github.com/openai/whisper/blob/main/whisper/decoding.py#L18-L69
    WHISPER_API int whisper_lang_auto_detect(
            struct whisper_context * ctx,
                           int   offset_ms,
                           int   n_threads,
                         float * lang_probs);

    WHISPER_API int whisper_lang_auto_detect_with_state(
            struct whisper_context * ctx,
              struct whisper_state * state,
                           int   offset_ms,
                           int   n_threads,
                         float * lang_probs);

    WHISPER_API int whisper_n_len          (struct whisper_context * ctx); // mel length
    WHISPER_API int whisper_n_len_from_state(struct whisper_state * state); // mel length

    WHISPER_API int whisper_n_vocab       (struct whisper_context * ctx);
    WHISPER_API int whisper_n_text_ctx    (struct whisper_context * ctx);
    WHISPER_API int whisper_n_audio_ctx   (struct whisper_context * ctx);
    WHISPER_API int whisper_is_multilingual(struct whisper_context * ctx);

    WHISPER_API int whisper_model_n_vocab      (struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_audio_ctx  (struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_audio_state(struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_audio_head (struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_audio_layer(struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_text_ctx   (struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_text_state (struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_text_head  (struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_text_layer (struct whisper_context * ctx);
    WHISPER_API int whisper_model_n_mels       (struct whisper_context * ctx);
    WHISPER_API int whisper_model_ftype        (struct whisper_context * ctx);
    WHISPER_API int whisper_model_type         (struct whisper_context * ctx);

    // Token logits obtained from the last call to whisper_decode()
    // The logits for the last token are stored in the last row
    // Rows: n_tokens
    // Cols: n_vocab
    WHISPER_API float * whisper_get_logits           (struct whisper_context * ctx);
    WHISPER_API float * whisper_get_logits_from_state(struct whisper_state * state);

    // Token Id -> String. Uses the vocabulary in the provided context
    WHISPER_API const char * whisper_token_to_str(struct whisper_context * ctx, whisper_token token);
    WHISPER_API whisper_token whisper_token_eot (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_sot (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_solm(struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_prev(struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_nosp(struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_not (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_beg (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_lang(struct whisper_context * ctx, int lang_id);

    // Task tokens
    WHISPER_API whisper_token whisper_token_translate (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_transcribe(struct whisper_context * ctx);

    // Performance information from the default state.
    WHISPER_API void whisper_print_timings(struct whisper_context * ctx);
    WHISPER_API void whisper_reset_timings(struct whisper_context * ctx);

    // Print system information
    WHISPER_API const char * whisper_print_system_info(void);

    ////////////////////////////////////////////////////////////////////////////

    // Create a new state independent of the default state included with the context
    // This can allow you to run multiple streams in parallel using the same model
    // You must call whisper_ctx_get_state() to get the default state of a context.
    WHISPER_API struct whisper_state * whisper_init_state(struct whisper_context * ctx);

    // Cleanup the specified state and free all allocated memory
    WHISPER_API void whisper_free_state(struct whisper_state * state);

    // Get the default state of the specified context
    // This state will be automatically freed when the context is freed
    WHISPER_API struct whisper_state * whisper_ctx_get_state(struct whisper_context * ctx);

    ////////////////////////////////////////////////////////////////////////////

    // Get the maximum value for language id, i.e. the number of available languages - 1
    WHISPER_API int whisper_lang_max_id();

#ifdef __cplusplus
}
#endif

#endif