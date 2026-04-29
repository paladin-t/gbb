#pragma bank 255
// Enable the above line for banked compilation.

#if defined __SDCC
#   pragma disable_warning 110
#   include <gb/hardware.h>
#else /* __SDCC */
#   error "Not implemented."
#endif /* __SDCC */

#include <string.h>

#include "speech.h"
#include "utils.h"

#if defined USE_SPEECH

#warning "Message: Compiling with speech synthesizer."

/*
** {===========================================================================
** Common
*/

#ifndef SPEECH_INTERNAL_STATIC
#   define SPEECH_INTERNAL_STATIC
#endif /* SPEECH_INTERNAL_STATIC */
#ifndef SPEECH_INTERNAL_API
#   define SPEECH_INTERNAL_API
#endif /* SPEECH_INTERNAL_API */

#ifndef SPEECH_TEXT_BUFFER_MAX_LENGTH
    // Sufficient for the longest pattern in `PHONEME_GRAPHEME_RULES`.
#   define SPEECH_TEXT_BUFFER_MAX_LENGTH   8
#endif /* SPEECH_TEXT_BUFFER_MAX_LENGTH */

/* ===========================================================================} */

/*
** {===========================================================================
** Phoneme
*/

/**< Declaration. */

#define PHONEME_NONE         0
#define PHONEME_VOWEL        1
#define PHONEME_CONSONANT    2
#define PHONEME_DIPHTHONG    3
#define PHONEME_SILENCE      4

#define PHONEME_MIN_FREQ     200
#define PHONEME_MAX_FREQ     1000

#define PHONEME_TABLE_SIZE   128

typedef struct {
    UINT8 symbol;
    UINT8 type;
    UINT8 freq;
    UINT8 duration;
    BOOLEAN voiced;
} Phoneme_t;

/**
 * Gets the phoneme data.
 *
 * @param sym The phoneme symbol.
 * @returns The pointer to the phoneme.
 */
const Phoneme_t * phoneme_get(UINT8 sym) SPEECH_INTERNAL_API;

/**
 * Converts text to phonemes.
 *
 * @param bank The bank of the input text.
 * @param txt The address of the input text.
 * @param len The length of the text.
 * @param curr_pos[in/out] Pointer to the current processing text position.
 * @param phonemes Pointer to the output phonemes buffer.
 * @param max_len The maximum length of the phonemes buffer.
 * @returns The phoneme count.
 */
UINT8 phoneme_text_to_next_phoneme(UINT8 bank, const char * txt, UINT16 len, UINT16 * curr_pos, char * phonemes, UINT8 max_len) SPEECH_INTERNAL_API;

/**
 * Convert normalized value to actual frequency.
 *
 * @param norm Normalized value (0-255).
 * @param f_min The minimum frequency.
 * @param f_max The maximum frequency.
 * @returns The actual frequency.
 */
UINT16 phoneme_norm_to_freq(UINT8 norm, UINT16 f_min, UINT16 f_max) SPEECH_INTERNAL_API;

/**< Implementation. */

SPEECH_INTERNAL_STATIC const Phoneme_t PHONEME_TABLE[PHONEME_TABLE_SIZE] = {
    //     Symbol,               type, freq, dur, voiced.
    // Vowels.
    ['a'] = { 'a',  PHONEME_VOWEL,      180,  12,   TRUE }, // ae as in "cat".
    ['A'] = { 'A',  PHONEME_VOWEL,      120,  14,   TRUE }, // A as in "father".
    ['e'] = { 'e',  PHONEME_VOWEL,      160,  10,   TRUE }, // e as in "bed".
    ['E'] = { 'E',  PHONEME_VOWEL,      100,  12,   TRUE }, // E as in "day".
    ['i'] = { 'i',  PHONEME_VOWEL,       80,  10,   TRUE }, // I as in "bit".
    ['I'] = { 'I',  PHONEME_VOWEL,       60,  14,   TRUE }, // i: as in "see".
    ['o'] = { 'o',  PHONEME_VOWEL,      140,  12,   TRUE }, // O as in "hot".
    ['O'] = { 'O',  PHONEME_VOWEL,      100,  14,   TRUE }, // oU as in "go".
    ['u'] = { 'u',  PHONEME_VOWEL,       80,  10,   TRUE }, // U as in "put".
    ['U'] = { 'U',  PHONEME_VOWEL,       60,  14,   TRUE }, // u: as in "too".
    ['V'] = { 'V',  PHONEME_VOWEL,      140,  10,   TRUE }, // V as in "cup".
    ['@'] = { '@',  PHONEME_VOWEL,      160,   8,   TRUE }, // @ as in "about".
    // Diphthongs.
    ['Y'] = { 'Y',  PHONEME_DIPHTHONG,  120,  16,   TRUE }, // aI as in "my".
    ['W'] = { 'W',  PHONEME_DIPHTHONG,  100,  16,   TRUE }, // aU as in "how".
    ['Q'] = { 'Q',  PHONEME_DIPHTHONG,  140,  14,   TRUE }, // OI as in "boy".
    // Nasals (voiced consonants).
    ['m'] = { 'm',  PHONEME_CONSONANT,  120,   8,   TRUE },
    ['n'] = { 'n',  PHONEME_CONSONANT,  140,   8,   TRUE },
    ['N'] = { 'N',  PHONEME_CONSONANT,  140,   8,   TRUE }, // N as in "sing".
    // Liquids.
    ['l'] = { 'l',  PHONEME_CONSONANT,  100,   6,   TRUE },
    ['r'] = { 'r',  PHONEME_CONSONANT,   80,   6,   TRUE },
    // Glides.
    ['w'] = { 'w',  PHONEME_CONSONANT,   60,   4,   TRUE },
    ['j'] = { 'j',  PHONEME_CONSONANT,   60,   4,   TRUE }, // y.
    // Plosives.
    ['p'] = { 'p',  PHONEME_CONSONANT,    0,   4,  FALSE },
    ['b'] = { 'b',  PHONEME_CONSONANT,   80,   4,   TRUE },
    ['t'] = { 't',  PHONEME_CONSONANT,    0,   4,  FALSE },
    ['d'] = { 'd',  PHONEME_CONSONANT,  100,   4,   TRUE },
    ['k'] = { 'k',  PHONEME_CONSONANT,    0,   4,  FALSE },
    ['g'] = { 'g',  PHONEME_CONSONANT,   80,   4,   TRUE },
    // Fricatives.
    ['f'] = { 'f',  PHONEME_CONSONANT,    0,   8,  FALSE },
    ['v'] = { 'v',  PHONEME_CONSONANT,   80,   8,   TRUE },
    ['T'] = { 'T',  PHONEME_CONSONANT,    0,   6,  FALSE }, // th as in "think".
    ['D'] = { 'D',  PHONEME_CONSONANT,  100,   6,   TRUE }, // th as in "this".
    ['s'] = { 's',  PHONEME_CONSONANT,    0,   8,  FALSE },
    ['z'] = { 'z',  PHONEME_CONSONANT,   80,   8,   TRUE },
    ['S'] = { 'S',  PHONEME_CONSONANT,    0,   8,  FALSE }, // sh as in "ship".
    ['Z'] = { 'Z',  PHONEME_CONSONANT,   80,   8,   TRUE }, // zh as in "vision".
    ['h'] = { 'h',  PHONEME_CONSONANT,   60,   4,  FALSE },
    // Affricates.
    ['C'] = { 'C',  PHONEME_CONSONANT,    0,  10,  FALSE }, // ch as in "church".
    ['J'] = { 'J',  PHONEME_CONSONANT,   80,  10,   TRUE }, // j as in "judge".
    // Specials.
    [' '] = { ' ',  PHONEME_SILENCE,      0,   4,  FALSE },
    ['.'] = { '.',  PHONEME_SILENCE,      0,  12,  FALSE },
    [','] = { ',',  PHONEME_SILENCE,      0,   8,  FALSE },
    ['!'] = { '!',  PHONEME_SILENCE,      0,  10,  FALSE },
    ['?'] = { '?',  PHONEME_SILENCE,      0,  10,  FALSE }
};

typedef struct {
    const char * pattern; // Letter pattern.
    UINT8 pattern_length;
    const char * phonemes;
} GraphemeRule_t;

SPEECH_INTERNAL_STATIC const GraphemeRule_t PHONEME_GRAPHEME_RULES[] = {
    // Digraphs.
    { "th",      2, "T"      }, { "TH", 2, "T"  }, { "Th", 2, "T"  },
    { "sh",      2, "S"      }, { "SH", 2, "S"  }, { "Sh", 2, "S"  },
    { "ch",      2, "C"      }, { "CH", 2, "C"  }, { "Ch", 2, "C"  },
    { "ng",      2, "N"      }, { "NG", 2, "N"  }, { "Ng", 2, "N"  },
    { "ck",      2, "k"      }, { "CK", 2, "k"  },
    { "ph",      2, "f"      }, { "PH", 2, "f"  }, { "Ph", 2, "f"  },
    { "wh",      2, "w"      }, { "WH", 2, "w"  }, { "Wh", 2, "w"  },
    // Vowel combinations (2 chars).
    { "ee",      2, "I"      }, { "EE", 2, "I"  }, { "Ee", 2, "I"  },
    { "oo",      2, "U"      }, { "OO", 2, "U"  }, { "Oo", 2, "U"  },
    { "ou",      2, "W"      }, { "OU", 2, "W"  }, { "Ou", 2, "W"  },
    { "ow",      2, "W"      }, { "OW", 2, "W"  }, { "Ow", 2, "W"  },
    { "ai",      2, "Y"      }, { "AI", 2, "Y"  }, { "Ai", 2, "Y"  },
    { "ay",      2, "Y"      }, { "AY", 2, "Y"  }, { "Ay", 2, "Y"  },
    { "oi",      2, "Q"      }, { "OI", 2, "Q"  }, { "Oi", 2, "Q"  },
    { "oy",      2, "Q"      }, { "OY", 2, "Q"  }, { "Oy", 2, "Q"  },
    { "ea",      2, "E"      }, { "EA", 2, "E"  }, { "Ea", 2, "E"  },
    { "ie",      2, "I"      }, { "IE", 2, "I"  }, { "Ie", 2, "I"  },
    { "oa",      2, "O"      }, { "OA", 2, "O"  }, { "Oa", 2, "O"  },
    { "er",      2, "@r"     }, { "ER", 2, "@r" }, { "Er", 2, "@r" },
    { "ir",      2, "@r"     }, { "IR", 2, "@r" }, { "Ir", 2, "@r" },
    { "ur",      2, "@r"     }, { "UR", 2, "@r" }, { "Ur", 2, "@r" },
    { "ar",      2, "Ar"     }, { "AR", 2, "Ar" }, { "Ar", 2, "Ar" },
    { "or",      2, "Or"     }, { "OR", 2, "Or" }, { "Or", 2, "Or" },
    // Suffix rules (2 chars).
    { "ed",      2, "d"      },
    { "es",      2, "z"      },
    { "ly",      2, "lI"     },
    // Suffix rules (3-4 chars).
    { "ing",     3, "IN"     },
    { "est",     3, "Ist"    },
    { "tion",    4, "S@n"    },
    // Common words.
    { "is",      2, "Iz"     }, { "Is",      2, "Iz"     },
    { "it",      2, "It"     }, { "It",      2, "It"     },
    { "to",      2, "tU"     }, { "To",      2, "tU"     },
    { "of",      2, "Vf"     }, { "Of",      2, "Vf"     },
    { "she",     3, "SI"     }, { "She",     3, "SI"     },
    { "the",     3, "D@"     }, { "The",     3, "D@"     },
    { "how",     3, "hW"     }, { "How",     3, "hW"     },
    { "are",     3, "Ar"     }, { "Are",     3, "Ar"     },
    { "but",     3, "bVt"    }, { "But",     3, "bVt"    },
    { "not",     3, "nt"     }, { "Not",     3, "nt"     },
    { "all",     3, "Ol"     }, { "All",     3, "Ol"     },
    { "can",     3, "kn"     }, { "Can",     3, "kn"     },
    { "her",     3, "h@r"    }, { "Her",     3, "h@r"    },
    { "was",     3, "wz"     }, { "Was",     3, "wz"     },
    { "one",     3, "wVn"    }, { "One",     3, "wVn"    },
    { "our",     3, "Wr"     }, { "Our",     3, "Wr"     },
    { "out",     3, "Wt"     }, { "Out",     3, "Wt"     },
    { "and",     3, "nd"     }, { "And",     3, "nd"     },
    { "for",     3, "fOr"    }, { "For",     3, "fOr"    },
    { "you",     3, "jU"     }, { "You",     3, "jU"     },
    { "bye",     3, "bY"     }, { "Bye",     3, "bY"     },
    { "that",    4, "Dt"     }, { "That",    4, "Dt"     },
    { "this",    4, "Ds"     }, { "This",    4, "Ds"     },
    { "with",    4, "wIT"    }, { "With",    4, "wIT"    },
    { "have",    4, "hv"     }, { "Have",    4, "hv"     },
    { "nice",    4, "nYs"    }, { "Nice",    4, "nYs"    },
    { "meet",    4, "mIt"    }, { "Meet",    4, "mIt"    },
    { "very",    4, "v@rI"   }, { "Very",    4, "v@rI"   },
    { "much",    4, "mVC"    }, { "Much",    4, "mVC"    },
    { "good",    4, "gUd"    }, { "Good",    4, "gUd"    },
    { "game",    4, "gEm"    }, { "Game",    4, "gEm"    },
    { "hello",   5, "h@lO"   }, { "Hello",   5, "h@lO"   },
    { "world",   5, "w@rld"  }, { "World",   5, "w@rld"  },
    { "thank",   5, "TNk"    }, { "Thank",   5, "TNk"    },
    { "welcome", 7, "w@lk@m" }, { "Welcome", 7, "w@lk@m" },
    { "we",      2, "wI"     }, { "We",      2, "wI"     },
    { "he",      2, "hI"     }, { "He",      2, "hI"     },
    // NULL terminator.
    { NULL,      0, NULL     }
};

const Phoneme_t * phoneme_get(UINT8 sym) SPEECH_INTERNAL_API {
    if (sym < PHONEME_TABLE_SIZE) {
        const Phoneme_t * p = &PHONEME_TABLE[sym];
        if (p->symbol)
            return p;
    }

    // Return silence by default.
    return &PHONEME_TABLE[' '];
}

UINT8 phoneme_text_to_next_phoneme(UINT8 bank, const char * txt, UINT16 len, UINT16 * curr_pos, char * phonemes, UINT8 max_len) SPEECH_INTERNAL_API {
    // Prepare.
    UINT16 in_pos = *curr_pos;
    UINT8 out_pos = 0;

    // Iterate the characters.
    if (in_pos < len) {
        // Get text into buffer.
        char buf[SPEECH_TEXT_BUFFER_MAX_LENGTH];
        const UINT8 buf_len = MIN(SPEECH_TEXT_BUFFER_MAX_LENGTH - 1, len - in_pos);
        get_chunk(buf, bank, (UINT8 *)txt + in_pos, buf_len);
        buf[buf_len] = '\0';

        // Try to match.
        BOOLEAN matched = FALSE;
        if (buf[0] != ' ' && buf[0] != '.' && buf[0] != ',' && buf[0] != '!' && buf[0] != '?') {
            for (UINT8 rule_idx = 0; PHONEME_GRAPHEME_RULES[rule_idx].pattern != NULL; ++rule_idx) {
                const GraphemeRule_t * rule = &PHONEME_GRAPHEME_RULES[rule_idx];
                const char * pattern = rule->pattern;
                const UINT8 pattern_len = rule->pattern_length;

                // Check for match.
                if (in_pos + pattern_len <= len) {
                    BOOLEAN match = TRUE;
                    for (UINT8 i = 0; i < pattern_len; ++i) {
                        if (buf[i] != pattern[i]) {
                            match = FALSE;

                            break;
                        }
                    }

                    if (match) {
                        // Copy phonemes.
                        const char * ph = rule->phonemes;
                        while (*ph && out_pos < max_len - 1)
                            phonemes[out_pos++] = *ph++;
                        in_pos += pattern_len;
                        matched = TRUE;

                        break;
                    }
                }
            }
        }

        // No rule matched, use character directly.
        if (!matched) {
            // Convert to lowercase.
            char c = buf[0];
            if (c >= 'A' && c <= 'Z') {
                // Keep uppercase vowels to distinguish long vowels.
                if (c != 'A' && c != 'E' && c != 'I' && c != 'O' && c != 'U')
                    c = c - 'A' + 'a';
            }
            phonemes[out_pos++] = c;
            ++in_pos;
        }
    }

    // Finish.
    phonemes[out_pos] = '\0';

    *curr_pos = in_pos;

    return out_pos;
}

UINT16 phoneme_norm_to_freq(UINT8 norm, UINT16 f_min, UINT16 f_max) SPEECH_INTERNAL_API {
    const UINT32 range = (UINT32)(f_max - f_min);
    const UINT32 result = f_min + (norm * range) / 255;

    return (UINT16)result;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Formant
*/

/**< Declaration. */

// Initializes the required audio hardware.
void formant_init(BOOLEAN init_aud_dev) SPEECH_INTERNAL_API;
// Mutes the channel.
void formant_mute(void) SPEECH_INTERNAL_API;
// Synthesizes voiced formant.
void formant_synthesize_voiced(UINT16 f0, UINT16 f1, UINT8 vol) SPEECH_INTERNAL_API;
// Synthesizes unvoiced formant.
void formant_synthesize_unvoiced(UINT16 f0, UINT16 f1, UINT8 vol) SPEECH_INTERNAL_API;
// Synthesizes silence.
void formant_synthesize_silence(void) SPEECH_INTERNAL_API;

/**< Implementation. */

// Sets the frequency.
SPEECH_INTERNAL_STATIC void formant_set_freq(UINT16 freq) {
    freq = CLAMP(freq, 64, 2000);
    const UINT16 freq_reg = MIN(2048 - (UINT16)(131072ul / freq), 2047);
    NR23_REG = freq_reg & 0xFF;
    NR24_REG = 0x80 | ((freq_reg >> 8) & 0x07);
}
// Sets the volume.
SPEECH_INTERNAL_STATIC void formant_set_volume(UINT8 vol) {
    NR22_REG = (vol << 4) | 0x08;
}

void formant_init(BOOLEAN init_aud_dev) SPEECH_INTERNAL_API {
    if (init_aud_dev) {
        NR52_REG = 0x80;
        NR51_REG = 0xFF;
        NR50_REG = 0x77;
    }
    NR21_REG = 0x40;
    NR22_REG = 0x00;
    NR23_REG = 0x00;
    NR24_REG = 0x80;
}

void formant_mute(void) SPEECH_INTERNAL_API {
    NR22_REG = 0x00;
}

void formant_synthesize_voiced(UINT16 f0, UINT16 f1, UINT8 vol) SPEECH_INTERNAL_API {
    if (f0 > 0) {
        const UINT16 freq = DIV2(f0) + DIV128((UINT32)DIV2(f1) * f0);
        formant_set_freq(freq);
        formant_set_volume(vol);
    } else {
        NR22_REG = 0x00;
    }
}

void formant_synthesize_unvoiced(UINT16 f0, UINT16 f1, UINT8 vol) SPEECH_INTERNAL_API {
    f1 /= 3;
    if (f1 > 0) {
        const UINT16 freq = DIV2(f1) + DIV128((UINT32)DIV2(f0) * f1);
        formant_set_freq(freq);
        formant_set_volume(vol);
    } else {
        NR22_REG = 0x00;
    }
}

void formant_synthesize_silence(void) SPEECH_INTERNAL_API {
    formant_mute();
}

/* ===========================================================================} */

/*
** {===========================================================================
** Speech
*/

SPEECH_STATIC SpeechSynth_t speech_synth;

// Sine lookup table for pitch jitter.
SPEECH_STATIC const INT8 SPEECH_SINE_TABLE[8] = { 0, 4, 7, 5, 0, -5, -7, -4 }; // 8-point sine wave, range -7 to +7.

SPEECH_STATIC UINT16 speech_get_pitch_target(UINT16 pos, UINT16 total) {
    // Prepare.
    const UINT16 base = speech_synth.pitch_base;
    INT16 offset = 0;

    // Calculate offset based on intonation pattern.
    switch (speech_synth.intonation) {
    case INTONE_STATEMENT: // Declarative: high start, gradually falling.
        offset = 40 - ((INT16)pos * 60) / (INT16)total;

        break;
    case INTONE_QUESTION: // Interrogative: low start, rising at the end.
        if (pos < DIV2(total))
            offset = 10;
        else
            offset = 10 + ((INT16)(pos - DIV2(total)) * 40) / (INT16)DIV2(total);

        break;
    case INTONE_EXCLAMATION: // Exclamatory: high start, rapid fall.
        offset = 60 - ((INT16)pos * 80) / (INT16)total;

        break;
    case INTONE_NEUTRAL: // Neutral: flat tone.
        offset = 20;

        break;
    }

    // Add slight jitter.
    offset += SPEECH_SINE_TABLE[pos & 0x07];

    // Finish.
    return (UINT16)((INT16)base + offset);
}

SPEECH_STATIC void speech_update_formants(void) {
    // Linear interpolation transition of frequency.
    if (speech_synth.freq_state != speech_synth.freq_target) {
        const INT16 diff = (INT16)speech_synth.freq_target - (INT16)speech_synth.freq_state;
        speech_synth.freq_state = (UINT16)((INT16)speech_synth.freq_state + DIV4(diff));
    }

    // Linear interpolation transition of pitch.
    if (speech_synth.pitch_state != speech_synth.pitch_target) {
        const INT16 diff = (INT16)speech_synth.pitch_target - (INT16)speech_synth.pitch_state;
        speech_synth.pitch_state = (UINT16)((INT16)speech_synth.pitch_state + DIV2(diff));
    }
}

SPEECH_STATIC void speech_synthesize_current_phoneme(UINT8 vol) {
    // Update formants.
    speech_update_formants();

    // Synthesize based on phoneme type.
    if (speech_synth.phoneme_type == PHONEME_SILENCE) {
        formant_synthesize_silence();
    } else if (speech_synth.phoneme_type == PHONEME_CONSONANT && !speech_synth.phoneme_voiced) {
        // Unvoiced consonant.
        formant_synthesize_unvoiced(
            speech_synth.pitch_state,
            speech_synth.freq_state,
            vol
        );
    } else {
        // Voiced sound (vowel/voiced consonant).
        formant_synthesize_voiced(
            speech_synth.pitch_state,
            speech_synth.freq_state,
            vol
        );
    }
}

void speech_init(BOOLEAN init_aud_dev) SPEECH_API {
    memset(&speech_synth, 0, sizeof(speech_synth));
    speech_synth.state        = SYNTH_IDLE;
    speech_synth.freq_state   = 500;
    speech_synth.pitch_base   = 120;
    speech_synth.pitch_state  = 120;
    speech_synth.pitch_target = 120;
    speech_synth.speed        = 5;
    speech_synth.intonation   = INTONE_AUTO;
    speech_synth.volume       = 14;

    formant_init(init_aud_dev);
}

void speech_set_volume(UINT8 vol) SPEECH_API {
    speech_synth.volume = MIN(vol, 15); // Clamp to 0-15.
}

void speech_set_speed(UINT8 speed) SPEECH_API {
    if (speed >= 1 && speed <= 10)
        speech_synth.speed = speed;
}

void speech_set_pitch(UINT16 pitch) SPEECH_API {
    speech_synth.pitch_base = pitch;

    // Also update current pitch state and target so the change takes effect immediately, even during playback.
    speech_synth.pitch_state = pitch;
    speech_synth.pitch_target = pitch;
}

void speech_set_intonation(IntonationPattern_t pattern) SPEECH_API {
    speech_synth.intonation = (UINT8)pattern;
}

BOOLEAN speech_is_playing(void) SPEECH_API {
    return (speech_synth.state != SYNTH_IDLE) ? TRUE : FALSE;
}

UINT8 speech_play(UINT8 bank, const char * txt, UINT16 len) SPEECH_API {
    // Check the state.
    if (speech_synth.state != SYNTH_IDLE)
        return 1; // Synthesizer busy.

    // Check the buffer.
    if (!txt || len == 0)
        return 2; // Empty buffer.

    // Reset the registers.
    NR21_REG = 0x40;
    NR22_REG = 0x00;
    NR23_REG = 0x00;
    NR24_REG = 0x80;

    // Set the text.
    speech_synth.text_bank = bank;
    speech_synth.text_address = txt;
    speech_synth.text_length = len;

    // Convert to phonemes.
    speech_synth.text_previous_cursor = 0;
    speech_synth.text_cursor = 0;
    speech_synth.phoneme_count = phoneme_text_to_next_phoneme(
        speech_synth.text_bank, speech_synth.text_address, speech_synth.text_length,
        &speech_synth.text_cursor,
        speech_synth.phoneme_buffer,
        SPEECH_PHONEME_MAX_COUNT
    );
    speech_synth.phoneme_cursor = 0;

    // Auto-detect intonation from ending punctuation, but only if the user hasn't explicitly set a pattern.
    if (speech_synth.intonation == INTONE_AUTO) {
        const char last = get_uint8(speech_synth.text_bank, (UINT8 *)speech_synth.text_address + speech_synth.text_length - 1);
        if (last == '.')
            speech_synth.intonation = INTONE_STATEMENT;
        else if (last == '?')
            speech_synth.intonation = INTONE_QUESTION;
        else if (last == '!')
            speech_synth.intonation = INTONE_EXCLAMATION;
        else
            speech_synth.intonation = INTONE_NEUTRAL;
    }

    // Reset the state.
    speech_synth.state = SYNTH_PROCESSING;

    return 0;
}

void speech_stop(void) SPEECH_API {
    formant_mute();
    speech_synth.state = SYNTH_IDLE;
    speech_synth.phoneme_cursor = 0;
}

void speech_update(void) SPEECH_API {
    switch (speech_synth.state) {
    case SYNTH_IDLE:
        // Do nothing.

        break;
    case SYNTH_PROCESSING:
        // Prepare the next phoneme.
        if (speech_synth.phoneme_cursor >= speech_synth.phoneme_count) {
            if (speech_synth.text_cursor < speech_synth.text_length) {
                // Next phoneme.
                speech_synth.text_previous_cursor = speech_synth.text_cursor;
                speech_synth.phoneme_count = phoneme_text_to_next_phoneme(
                    speech_synth.text_bank, speech_synth.text_address, speech_synth.text_length,
                    &speech_synth.text_cursor,
                    speech_synth.phoneme_buffer,
                    SPEECH_PHONEME_MAX_COUNT
                );
                speech_synth.phoneme_cursor = 0;
            } else {
                // All phonemes processed.
                speech_synth.state = SYNTH_IDLE;
                formant_mute();

                return;
            }
        }

        // Get the current phoneme data.
        {
            const Phoneme_t * p = phoneme_get(speech_synth.phoneme_buffer[speech_synth.phoneme_cursor]);
            speech_synth.phoneme_type = p->type;
            speech_synth.phoneme_voiced = p->voiced;

            // Set the formant target.
            speech_synth.freq_target = phoneme_norm_to_freq(p->freq, PHONEME_MIN_FREQ, PHONEME_MAX_FREQ);

            // Set the pitch target.
            const UINT16 interval = speech_synth.text_cursor - speech_synth.text_previous_cursor;
            const UINT16 elapsed = interval * speech_synth.phoneme_cursor / speech_synth.phoneme_count;
            speech_synth.pitch_target = speech_get_pitch_target(
                speech_synth.text_previous_cursor + elapsed,
                speech_synth.text_length
            );

            // Set the duration adjusted by speaking rate.
            speech_synth.phoneme_duration = (p->duration * 10) / MUL2(speech_synth.speed);
            speech_synth.phoneme_duration = MAX(speech_synth.phoneme_duration, 2);
            speech_synth.phoneme_timer = speech_synth.phoneme_duration;
        }

        // Update the state.
        speech_synth.state = SYNTH_PLAYING;

        break;
    case SYNTH_PLAYING:
        // Synthesize the current phoneme.
        speech_synthesize_current_phoneme(speech_synth.volume);

        // Count down.
        if (speech_synth.phoneme_timer > 0)
            --speech_synth.phoneme_timer;

        // Phoneme finished, move to the next.
        if (speech_synth.phoneme_timer == 0) {
            ++speech_synth.phoneme_cursor;
            speech_synth.state = SYNTH_PROCESSING;
        }

        break;
    }
}

/* ===========================================================================} */

#endif /* USE_SPEECH */
