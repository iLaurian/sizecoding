#include <stdio.h>
#include <math.h>
#include "song.h"

#define RATE   44100            // samples per second
#define VOICES 16               // how many strings can ring
#define BOWS   8                // how many bowed notes can sound
#define MAXLEN 2048             // longest string (lowest note)
#define STEP (RATE * 60 / (SONG_BPM * 4))

typedef struct {
    float buf[MAXLEN];          // the string's vibration
    int len;                    // buffer length -> pitch
    int pos;                    // current position in the loop
} String;

static String str[VOICES];
static int next_voice;
static unsigned seed = 1;

// random number between -1 and +1
static float noise(void)
{
    seed = seed * 1103515245 + 12345;
    return ((seed >> 16) & 0x7fff) / 16384.0f - 1;
}

// MIDI note number -> frequency in Hz (69 = A = 440 Hz)
static float freq(int note)
{
    return 440 * powf(2, (note - 69) / 12.0f);
}

// snap the next free string: fill it with noise
static void pluck(int note)
{
    String *s = &str[next_voice];
    next_voice = (next_voice + 1) % VOICES;
    s->len = (int)(RATE / freq(note));
    s->pos = 0;
    for (int i = 0; i < s->len; i++)
        s->buf[i] = noise();
}

// one sample from one string, then smooth that spot (Karplus-Strong)
static float string_step(String *s)
{
    if (s->len == 0) return 0;
    int i = s->pos, j = (i + 1) % s->len;
    float out = s->buf[i];
    s->buf[i] = (s->buf[i] + s->buf[j]) * 0.5f * 0.997f;
    s->pos = j;
    return out;
}

typedef struct {
    float f;                    // frequency in Hz; 0 = this bow is free
    float ph1, ph2;             // the two sawtooth phases (0..1)
    float lp;                   // the smoothing filter's memory
    int t;                      // age of the note, in samples
    int hold;                   // how long to hold, in samples
} Bow;

static Bow bow[BOWS];
static int next_bow;

// start a bowed note on the next voice, held for `steps` 16th notes
static void bow_start(int note, int steps)
{
    Bow *b = &bow[next_bow];
    next_bow = (next_bow + 1) % BOWS;
    b->f = freq(note);
    b->t = 0;
    b->hold = steps * STEP;
}

// one sample from one bowed note
static float bow_step(Bow *b)
{
    if (b->f == 0) return 0;                            // free voice: silent

    float sec = (float)b->t / RATE;                     // note age in seconds
    float vib = 1 + 0.004f * sinf(sec * 2 * M_PI * 5.5f);   // vibrato: +-0.4%

    // two sawtooths, the second 0.4% sharper -> a section, not a soloist
    b->ph1 += b->f * vib / RATE;
    b->ph1 -= floorf(b->ph1);
    b->ph2 += b->f * vib * 1.004f / RATE;
    b->ph2 -= floorf(b->ph2);
    float saw = (b->ph1 * 2 - 1) + (b->ph2 * 2 - 1);

    // soften the buzz: move only 8% of the way toward the new value
    b->lp += (saw - b->lp) * 0.08f;

    // envelope: swell in over 0.12 s, hold, then fade out
    float env = fminf(sec / 0.12f, 1);
    if (b->t > b->hold)
        env *= expf(-(float)(b->t - b->hold) / (0.25f * RATE));

    if (b->t > b->hold + RATE) b->f = 0;                // 1 s after release: free
    b->t++;
    return b->lp * env;
}

static int pi, pwaited;
static int bi, biwaited;
static int step, in_step;

static float music_sample(void) {
    if (in_step == 0 && step < SONG_STEPS) {
	while (pi < (int)sizeof pluck_data && pwaited == pluck_data[pi]) {
	    if (pluck_data[pi + 1])
		pluck(pluck_data[pi + 1]);
	    pi += 2;
	    pwaited = 0;
	}
	pwaited++;

	while (bi < (int)sizeof bow_data && biwaited == bow_data[bi]) {
	    if (bow_data[bi + 1])
		bow_start(bow_data[bi + 1], bow_data[bi + 2]);
	    bi += 3;
	    biwaited = 0;
	}
	biwaited++;
    }

    if (++in_step == STEP) {
	in_step = 0;
	step++;
    }

    float p = 0, b = 0;
    for (int v = 0; v < VOICES; v++)
	p += string_step(&str[v]);
    for (int v = 0; v < BOWS; v++)
	b += bow_step(&bow[v]);
    return 0.19f * p + 0.06f * b;
}

#ifndef NO_MAIN

static void out(float s)
{
    if (s > 1) s = 1;
    if (s < -1) s = -1;
    short v = (short)(s * 32767);
    fwrite(&v, 2, 1, stdout);
}

int main(void)
{
    for (long k = 0; k < (long)SONG_STEPS * STEP; k++)
	out(music_sample());
    return 0;
}
#endif
