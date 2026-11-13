#include <math.h>
#include "sys.h"
#include "song.h"

#define RATE   44100
#define VOICES 16
#define BOWS   8
#define MAXLEN 2048
#define STEP (RATE * 60 / (SONG_BPM * 4))

typedef struct {
    float buf[MAXLEN];
    int len;
    int pos;
} String;

static String str[VOICES];
static int next_voice;
static unsigned seed = 1;

static float noise(void)
{
    seed = seed * 1103515245 + 12345;
    return ((seed >> 16) & 0x7fff) / 16384.0f - 1;
}

static float freq(int note)
{
    return 440 * ex2((note - 69) / 12.0f);
}

static void pluck(int note)
{
    String *s = &str[next_voice];
    next_voice = (next_voice + 1) % VOICES;
    s->len = (int)(RATE / freq(note));
    s->pos = 0;
    for (int i = 0; i < s->len; i++)
        s->buf[i] = noise();
}

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
    float f;
    float ph1, ph2;
    float lp;
    int t;
    int hold;
} Bow;

static Bow bow[BOWS];
static int next_bow;

static void bow_start(int note, int steps)
{
    Bow *b = &bow[next_bow];
    next_bow = (next_bow + 1) % BOWS;
    b->f = freq(note);
    b->t = 0;
    b->hold = steps * STEP;
}

static float bow_step(Bow *b)
{
    if (b->f == 0) return 0;

    float sec = (float)b->t / RATE;
    float vib = 1 + 0.004f * sinf(sec * 2 * M_PI * 5.5f);

    b->ph1 += b->f * vib / RATE;
    b->ph1 -= (int)b->ph1;
    b->ph2 += b->f * vib * 1.004f / RATE;
    b->ph2 -= (int)b->ph2;
    float saw = (b->ph1 * 2 - 1) + (b->ph2 * 2 - 1);

    b->lp += (saw - b->lp) * 0.08f;

    float env = fminf(sec / 0.12f, 1);
    if (b->t > b->hold)
        env *= ex(-(float)(b->t - b->hold) / (0.25f * RATE));

    if (b->t > b->hold + RATE) b->f = 0;
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
int main(void)
{
    static short buf[4096];
    int n = 0;
    for (long k = 0; k < (long)SONG_STEPS * STEP; k++) {
	float s = fminf(fmaxf(music_sample(), -1), 1);
	buf[n++] = (short)(s * 32767);
	if (n == 4096) {
	    write_all(1, buf, sizeof buf);
	    n = 0;
	}
    }
    write_all(1, buf, n * 2);
    return 0;
}
#endif
