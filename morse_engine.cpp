#include "morse_engine.h"
#include <string.h>
#include <Arduino.h>

static const char* MORSE_TABLE[26] = {
    ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",
    "....", "..",   ".---", "-.-",  ".-..", "--",   "-.",
    "---",  ".--.", "--.-", ".-.",  "...",  "-",    "..-",
    "...-", ".--",  "-..-", "-.--", "--.."
};

static bool append(MorseSequence* s, uint16_t dur, bool on) {
    if (s->count >= MAX_MORSE_SLOTS) return false;
    s->slots[s->count].duration_ms = dur;
    s->slots[s->count].on          = on;
    s->count++;
    return true;
}

bool morse_encode(const char* text, MorseSequence* out) {
    if (!text || !out) return false;
    out->count = 0;

    for (int i = 0; text[i]; i++) {
        char c = text[i];
        if (c != ' ' && (c < 'a' || c > 'z')) return false;
    }

    const uint16_t T = MORSE_T_MS;
    bool first_letter = true;

    // Perceptual start gap
    if (!append(out, 3 * T, false)) return false;

    for (int i = 0; text[i]; i++) {
        char c = text[i];

        if (c == ' ') {
            if (!first_letter) {
                if (!append(out, 4 * T, false)) return false;
            } else {
                if (!append(out, 7 * T, false)) return false;
            }
            continue;
        }

        if (!first_letter) {
            if (!append(out, 3 * T, false)) return false;  // letter gap
        }
        first_letter = false;

        const char* code = MORSE_TABLE[c - 'a'];
        for (int j = 0; code[j]; j++) {
            uint16_t on_dur = (code[j] == '-') ? 3 * T : T;
            if (!append(out, on_dur, true)) return false;
            if (code[j + 1] != '\0') {
                if (!append(out, T, false)) return false;  // intra-element gap
            }
        }
    }

    // Perceptual end gap
    if (!append(out, 3 * T, false)) return false;

    return true;
}

// ── Playback state management ──────────────────────────────────────────────

void morse_player_reset(MorsePlayer* p) {
    // Resets playback position ONLY. seq is intentionally preserved.
    p->index         = 0;
    p->active        = false;
    p->in_cooldown   = false;
    p->slot_start_ms = 0;
    p->cooldown_start = 0;
}

void morse_player_clear(MorsePlayer* p) {
    // Full teardown — also clears the sequence.
    morse_player_reset(p);
    p->seq.count = 0;
}

void morse_player_start(MorsePlayer* p) {
    p->index         = 0;
    p->active        = true;
    p->in_cooldown   = false;
    p->slot_start_ms = millis();
}

bool morse_player_tick(MorsePlayer* p, bool* out_on) {
    if (!p->active) {
        *out_on = false;
        return false;
    }

    unsigned long now = millis();

    // ── Cooldown phase ────────────────────────────────────────────────────
    if (p->in_cooldown) {
        if (now - p->cooldown_start >= MORSE_COOLDOWN_MS) {
            p->active      = false;
            p->in_cooldown = false;
            *out_on        = false;
            return false;   // done
        }
        *out_on = false;
        return true;
    }

    // ── Sequence exhausted ─────────────────────────────────────────────────
    if (p->index >= p->seq.count) {
        p->in_cooldown    = true;
        p->cooldown_start = now;
        *out_on           = false;
        return true;
    }

    // ── Advance at most ONE slot per tick ─────────────────────────────────
    // This prevents the while-loop race that previously consumed the entire
    // sequence in a single loop() iteration, making morse appear as a 1-second
    // blip rather than a full pattern.
    unsigned long elapsed = now - p->slot_start_ms;
    if (elapsed >= p->seq.slots[p->index].duration_ms) {
        p->slot_start_ms += p->seq.slots[p->index].duration_ms;
        p->index++;

        if (p->index >= p->seq.count) {
            p->in_cooldown    = true;
            p->cooldown_start = now;
            *out_on           = false;
            return true;
        }
    }

    *out_on = p->seq.slots[p->index].on;
    return true;
}
