#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define NO_MAIN
#include "sound.c"

#define W 120
#define H 60
#define HORIZON 40

#define SUN_X 85
#define SUN_Y (34 + T * 0.15f)
#define SUN_R 6

#define KX 40
#define KY 22

#define FPS 25
#define SPF (RATE / FPS)
#ifndef FRAMES
#define FRAMES (SONG_STEPS * STEP / SPF)
#endif

#define SKIP     (RATE * 5 / 4)  // start the music 1.25 s into the song
#define AV_DELAY 0.10f           // seconds between writing sound and hearing it
#define QUANT    4               // round colours to steps of 4
#ifndef AUDIO_CMD
#define AUDIO_CMD "pacat --raw --format=s16le --rate=44100 --channels=1 --latency-msec=100"
#endif

static float T;
unsigned char img[H][W][3];
static unsigned char prev[H][W][3];    // what is on screen right now

static int clamp(float v) { return v < 0 ? 0 : v > 255 ? 255 : (int)v; }

static void put(int x, int y, float r, float g, float b) {
    img[y][x][0] = clamp(r);
    img[y][x][1] = clamp(g);
    img[y][x][2] = clamp(b);
}

static float hash(int n) {
    n = (n << 13) ^ n;
    n = n * (n * n * 15731 + 789221) + 1376312589;
    return (n & 0x7fffffff) / 2147483647.0f;
}

static float circle(float x, float y, float cx, float cy, float r) {
    return sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy)) - r;
}

static float box(float x, float y, float cx, float cy, float hw, float hh) {
    float dx = fabsf(x - cx) - hw;
    float dy = fabsf(y - cy) - hh;
    return fmaxf(dx, dy);
}

static float capsule(float x, float y, float ax, float ay,
		     float bx, float by, float r) {
   float px = x - ax, py = y - ay;
   float vx = bx - ax, vy = by - ay;
   float t = (px * vx + py * vy) / (vx * vx + vy * vy);
   t = fminf(fmaxf(t, 0.0f), 1.0f);
   float dx = px - vx * t, dy = py - vy * t;
   return sqrtf(dx * dx + dy * dy) - r;
}

static float plume(float x, float y) {
    float d = 1e9f;
    for (int i = 0; i < 24; i++) {
	d = fminf(d, circle(x, y,
			    KX - i * 0.9f + sinf(T * 3 - i * 0.35f) * i * 0.06,
			    KY - 12 + i * i * 0.055f, 3.5f - i * 0.075f));
    }
    return d;
}

static float helmet(float x, float y) {
    float components[] = {
	circle(x, y, KX, KY, 11),
	box(x, y, KX, KY + 10, 11, 10),
	capsule(x, y, KX + 11, KY + 2, KX + 15, KY + 10, 1.5f),
	capsule(x, y, KX + 15, KY + 10, KX + 11, KY + 19, 1.5f),
	box(x, y, KX + 12, KY + 10, 2, 7),
    };
    float d = 1e9f;
    for (int i = 0; i < 5; i ++)
	d = fminf(d, components[i]);
    return d;
}

static float body(float x, float y) {
    float gorget = box(x, y, KX - 1, KY + 24, 9, 5);
    float shoulders = circle(x, y, KX - 6, KY + 50, 25);
    return fminf(gorget, shoulders);
}

static float knight(float x, float y) {
    return fminf(fminf(helmet(x, y), plume(x, y)), body(x, y));
}

// ---------------- sound: runs in its own process ----------------
static void audio_process(void) {
    FILE *audio = popen(AUDIO_CMD, "w");
    for (long k = 0; k < SKIP; k++)          // jump over part of the silent intro
	music_sample();
    for (long k = 0; k < (long)FRAMES * SPF; k++) {
	float t = (float)k / RATE;
	float fade = fminf(fmaxf((FRAMES / FPS - t) / 3, 0), 1);
	float s = fminf(fmaxf(music_sample() * fade, -1), 1);
	short v = (short)(s * 32767);
	fwrite(&v, 2, 1, audio);              // blocks while the player is full
    }
    pclose(audio);
    _exit(0);
}

// ---------------- picture: helpers ----------------
static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static int colour(unsigned char *p) {     // pack a pixel into one number, rounded
    return p[0] / QUANT * QUANT << 16 | p[1] / QUANT * QUANT << 8 | p[2] / QUANT * QUANT;
}

// print only the cells that changed since the last frame
static void draw(void) {
    for (int y = 0; y < H; y += 2) {
	int fg = -1, bg = -1, here = 0;
	for (int x = 0; x < W; x++) {
	    int t = colour(img[y][x]), b = colour(img[y + 1][x]);
	    if (t == colour(prev[y][x]) && b == colour(prev[y + 1][x])) {
		here = 0;                          // unchanged: skip it
		continue;
	    }
	    if (!here)                            // jump the cursor to this cell
		printf("\x1b[%d;%dH", y / 2 + 1, x + 1);
	    here = 1;
	    if (t != fg && b != bg)
		printf("\x1b[38;2;%d;%d;%d;48;2;%d;%d;%dm",
		       t >> 16, t >> 8 & 255, t & 255, b >> 16, b >> 8 & 255, b & 255);
	    else if (t != fg)
		printf("\x1b[38;2;%d;%d;%dm", t >> 16, t >> 8 & 255, t & 255);
	    else if (b != bg)
		printf("\x1b[48;2;%d;%d;%dm", b >> 16, b >> 8 & 255, b & 255);
	    fg = t;
	    bg = b;
	    printf("\xe2\x96\x80");
	}
    }
    memcpy(prev, img, sizeof img);
    fflush(stdout);
}

int main(void) {
    pid_t player = fork();
    if (player == 0)
	audio_process();                       // the child plays the music and exits
    double start = now();

    static char buf[1 << 20];
    setvbuf(stdout, buf, _IOFBF, sizeof buf);
    printf("\x1b[2J");
    printf("\x1b[?25l");
    memset(prev, 255, sizeof prev);          // force the first frame to draw fully

    for (int frame = 0; frame < FRAMES; ) {
	// which frame should be on screen now? (the sound is AV_DELAY behind)
	double t = now() - start - AV_DELAY;
	int want = t < 0 ? 0 : (int)(t * FPS);
	if (want < frame) {                     // too early: sleep until it's due
	    usleep((useconds_t)((frame / (double)FPS - t) * 1e6));
	    continue;
	}
	frame = want;                           // too late: skip ahead, never fall behind
	if (frame >= FRAMES)
	    break;
	T = (float)frame / FPS;
	for (int y = 0; y < H; y++) {
	    for (int x = 0; x < W; x++) {
		float r, g, b;
		int sx = x + (int)round(1.5f * sinf(T * 1.5 - x * 0.08));
		int tip = HORIZON - (int)(hash(sx) * 4);
		if (y < tip) {
		    float t = (float)y / HORIZON;
		    r = 110 + 145 * t;
		    g = 50 + 150 * t;
		    b = 20 + 60 * t;

		    float dsun = circle(x, y, SUN_X, SUN_Y, SUN_R);
		    if (dsun < 0) {
			r = 255; g = 250; b = 220;
		    } else {
			float glow = expf(-(dsun + SUN_R) * 0.12f);
			r += 120 * glow; g += 110 * glow; b += 50 * glow;
		    }
		} else {
		    float depth = (float)(y - HORIZON) / (H - HORIZON);
		    if (depth < 0) depth = 0;
		    r = 170 - 90 * depth;
		    g = 115 - 70 * depth;
		    b = 30 - 20 * depth;

		    int width = 1 + (int)(depth * 3);
		    float stalk = hash(sx / width + y / 7 * 1000);
		    float shade = 0.65f + 0.5f * stalk;
		    shade += 0.15f * (hash(x * 131 + y * 7919) - 0.5f);
		    float light = expf(-fabsf(x - SUN_X) * 0.06f) * (1 - depth);
		    r = r * shade + 110 * light;
		    g = g * shade + 80 * light;
		    b = b * shade + 20 * light;
		}
		if (knight(x, y) < 0) {
		    if (plume(x, y) < 0 && helmet(x, y) >= 0) {
			r = 20; g = 30; b = 70;
		    } else {
			r = 35; g = 55; b = 95;
		    }

		    float nx = knight(x + 1, y) - knight(x - 1, y);
		    float ny = knight(x, y + 1) - knight(x, y - 1);
		    float len = sqrtf(nx * nx + ny * ny) + 1e-6f;
		    nx /= len; ny /= len;
		    float light = nx * 0.9 + ny * (-0.4);
		    if (light < 0) light = 0;
		    r += 70 * light; g += 90 * light; b += 110 * light;

		    if (fabsf(circle(x, y, KX - 6, KY + 50, 25) + 5) < 0.6) {
			r *= 0.5; g *= 0.5; b *= 0.5;
		    }

		    if (box(x, y, KX + 9, KY + 7, 7, 0.6) < 0) {
			r = 5; g = 8; b = 15;
		    }

		    for (int i = 0; i < 3; i++) {
			if (circle(x, y, KX + 8 + i * 2.5f, KY + 13, 0.7f) < 0) {
			    r = 5; g = 8; b = 15;
			}
		    }

		    if (knight(x + 1.5f, y - 0.5f) >= 0) {
			r = 255; g = 175; b = 80;
		    }
		}

		float n = hash(x * 7 + y * 1031 + 0) - 0.5;
		r += 6 * n; g += 6 * n; b += 6 * n;

		float vx = (x - W/2.0f) / (W/2.0f);
		float vy = (y - H/2.0f) / (H/2.0f);
		float vignette = 1 - 0.25f * (vx * vx + vy * vy);
		float fade_in = fminf(T / 3, 1);
		float fade_out = fminf((FRAMES / FPS - T) / 3, 1);

		float above = (HORIZON - SUN_Y + SUN_R) / (2.0f * SUN_R);
		above = fminf(fmaxf(above, 0), 1);
		float dusk = 0.4f + 0.6f * above;

		float v = vignette * fade_in * fade_out * dusk;
		r *= v; g *= v; b *= v;

		if (y < 5 || y >= H - 5) {
		    r = 0; g = 0; b = 0;
		}

		put(x, y, r, g, b);
	    }
	}

	draw();
	frame++;
    }

    printf("\x1b[0m\x1b[?25h\x1b[%d;1H\n", H / 2 + 1);
    fflush(stdout);
    waitpid(player, NULL, 0);           // let the music finish
    return 0;
}
