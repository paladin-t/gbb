#ifndef __SPEECH_H__
#define __SPEECH_H__

#if defined __SDCC
#   include <gb/gb.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <stdint.h>

#ifndef USE_SPEECH
#   define USE_SPEECH 0
#endif /* USE_SPEECH */

#if USE_SPEECH

#ifndef SPEECH_BANKED
#   define SPEECH_BANKED              1
#endif /* SPEECH_BANKED */
#ifndef SPEECH_API
#   if SPEECH_BANKED
#      define SPEECH_API              BANKED
#   else /* SPEECH_BANKED */
#      define SPEECH_API
#   endif /* SPEECH_BANKED */
#endif /* SPEECH_API */
#ifndef SPEECH_STATIC
#   define SPEECH_STATIC
#endif /* SPEECH_STATIC */

#ifndef SPEECH_CHANNEL_MASK
    // The channel mask used by this speech module, (the second square channel, a.k.a. CH2).
#   define SPEECH_CHANNEL_MASK        0x02
#endif /* SPEECH_CHANNEL_MASK */

#ifndef SPEECH_PHONEME_MAX_COUNT
#   define SPEECH_PHONEME_MAX_COUNT   8
#endif /* SPEECH_PHONEME_MAX_COUNT */

typedef enum {
    SYNTH_IDLE,
    SYNTH_PROCESSING,
    SYNTH_PLAYING
} SynthState_t;

typedef enum {
    INTONE_STATEMENT,     // Declarative sentence - falling.
    INTONE_QUESTION,      // Interrogative sentence - rising.
    INTONE_EXCLAMATION,   // Exclamatory sentence - high falling.
    INTONE_NEUTRAL,       // Neutral - level tone.
    INTONE_AUTO           // Auto-detect from punctuation (default).
} IntonationPattern_t;

typedef struct {
    /**< Text buffer. */

    UINT8 text_bank;
    const char * text_address;
    UINT16 text_length;
    UINT16 text_previous_cursor;
    UINT16 text_cursor;

    /**< Phoneme buffer. */

    char phoneme_buffer[SPEECH_PHONEME_MAX_COUNT];
    UINT8 phoneme_count;
    UINT8 phoneme_cursor;

    /**< Current phoneme states. */

    UINT8 phoneme_type;
    UINT8 phoneme_voiced;
    UINT8 phoneme_duration;
    UINT8 phoneme_timer;

    /**< Current formant states. */

    UINT16 freq_state;
    UINT16 freq_target;

    /**< Pitch states. */

    UINT16 pitch_base;
    UINT16 pitch_state;
    UINT16 pitch_target;

    /**< Synthesis states. */

    UINT8 state;
    UINT8 speed;
    UINT8 intonation;
    UINT8 volume;
} SpeechSynth_t;

/**
 * @brief Initializes the speech synthesizer.
 *
 * @param init_aud_dev Whether to initialize the audio device.
 */
void speech_init(BOOLEAN init_aud_dev) SPEECH_API;

/**
 * @brief Sets the output volume.
 *
 * @param vol The volume (0-15), prefers 0-14 in practice, defaults to 14.
 */
void speech_set_volume(UINT8 vol) SPEECH_API;
/**
 * @brief Sets the speech speed.
 *
 * @param speed The speed factor (1 to 10), defaults to 5.
 */
void speech_set_speed(UINT8 speed) SPEECH_API;
/**
 * @brief Sets the base pitch.
 *
 * @param pitch Base pitch in Hz (i.e. male~120, female~200, child~300), defaults to 120.
 */
void speech_set_pitch(UINT16 pitch) SPEECH_API;
/**
 * @brief Sets the intonation pattern.
 *
 * @param pattern The intonation pattern, defaults to `INTONE_AUTO`.
 */
void speech_set_intonation(IntonationPattern_t pattern) SPEECH_API;

/**
 * @brief Gets whether the synthesizer is playing.
 *
 * @returns `TRUE` for playing.
 */
BOOLEAN speech_is_playing(void) SPEECH_API;
/**
 * @brief Speaks the specific text asynchronously.
 *
 * @param bank The bank of the text.
 * @param txt The text to speak.
 * @param len The length of the text.
 * @returns 0=ok, 1=busy, 2=nothing to synthesize.
 */
UINT8 speech_play(UINT8 bank, const char * txt, UINT16 len) SPEECH_API;
/**
 * @brief Stops speaking.
 */
void speech_stop(void) SPEECH_API;
/**
 * @brief Updates the synthesizer.
 */
void speech_update(void) SPEECH_API;

#endif /* USE_SPEECH */

#endif /* __SPEECH_H__ */
