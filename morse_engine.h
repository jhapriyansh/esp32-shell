#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MORSE_T_MS        80      // base unit ms (ITU standard)
#define MORSE_COOLDOWN_MS 1000    // cooldown after sequence ends
#define MAX_MORSE_SLOTS   256

struct MorseSlot {
    uint16_t duration_ms;
    bool     on;
};

struct MorseSequence {
    MorseSlot slots[MAX_MORSE_SLOTS];
    uint16_t  count;
};

// Encode a-z + spaces into a MorseSequence. Returns false if invalid/too long.
bool morse_encode(const char* text, MorseSequence* out);

struct MorsePlayer {
    MorseSequence seq;
    uint16_t      index;
    unsigned long slot_start_ms;
    bool          active;
    bool          in_cooldown;
    unsigned long cooldown_start;
};

void morse_player_reset(MorsePlayer* p);
void morse_player_clear(MorsePlayer* p);
void morse_player_start(MorsePlayer* p);

// Returns true while still playing (caller should use *out_on to drive hardware).
// Returns false when sequence + cooldown are both complete.
bool morse_player_tick(MorsePlayer* p, bool* out_on);
