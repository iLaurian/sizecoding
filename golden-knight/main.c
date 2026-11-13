#include <math.h>
#include "sys.h"

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
#define FRAMES (FPS * 60)
#endif

#define AV_DELAY 0.10f
#define QUANT    4

static float T;
unsigned char img[H][W][3];
static unsigned char prev[H][W][3];

static char out[1 << 20];
static int out_len;

static void put_c(char c) {
    out[out_len++] = c;
}

static void put_s(const char *s) {
    while (*s)
	put_c(*s++);
}

static void put_num(int v) {
    if (v >= 10)
	put_num(v / 10);
    put_c('0' + v % 10);
}

static void put_rgb(int c) {
    put_num(c >> 16);
    put_c(';');
    put_num(c >> 8 & 255);
    put_c(';');
    put_num(c & 255);
}


static void flush_out(void) {
    write_all(2, out, out_len);
    out_len = 0;
}

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

static void audio_process(void) {
    short chunk[SPF];
    for (long k = 0; k < (long)FRAMES * SPF;) {
	for (int i = 0; i < SPF; i ++, k++) {
	    float t = (float)k / RATE;
	    float fade = fminf(fmaxf((FRAMES / FPS - t) / 3, 0), 1);
	    float s = fminf(fmaxf(music_sample() * fade, -1), 1);
	    chunk[i] = (short)(s * 32767);
	}
	write_all(1, (char*)chunk, sizeof chunk);
    }
    sys3(SYS_EXIT, 0, 0, 0);
}

static int colour(unsigned char *p) {
    return p[0] / QUANT * QUANT << 16 | p[1] / QUANT * QUANT << 8 | p[2] / QUANT * QUANT;
}

static void draw(void) {
    for (int y = 0; y < H; y += 2) {
	int fg = -1, bg = -1, here = 0;
	for (int x = 0; x < W; x++) {
	    int t = colour(img[y][x]), b = colour(img[y + 1][x]);
	    if (t == colour(prev[y][x]) && b == colour(prev[y + 1][x])) {
		here = 0;
		continue;
	    }
	    if (!here) {
		put_s("\x1b[");
		put_num(y / 2 + 1);
		put_c(';');
		put_num(x + 1);
		put_c('H');
	    }
	    here = 1;
	    if (t != fg && b != bg) {
		put_s("\x1b[38;2;");
		put_rgb(t);
		put_s(";48;2;");
		put_rgb(b);
		put_c('m');
	    }
	    else if (t != fg) {
		put_s("\x1b[38;2;");
		put_rgb(t);
		put_c('m');
	    }
	    else if (b != bg) {
		put_s("\x1b[48;2;");
		put_rgb(b);
		put_c('m');
	    }
	    fg = t;
	    bg = b;
	    put_s("\xe2\x96\x80");
	}
    }
    put_s("\x1b[0m");
    __builtin_memcpy(prev, img, sizeof img);
    flush_out();
}

int main(void) {
    int player = sys3(SYS_FORK, 0, 0, 0);
    if (player == 0)
	audio_process();
    double start = now();

    put_s("\x1b[?1049h\x1b[0m\x1b[2J");
    put_s("\x1b[?25l");
    __builtin_memset(prev, 255, sizeof prev);

    for (int frame = 0; frame < FRAMES; ) {
	double t = now() - start - AV_DELAY;
	int want = t < 0 ? 0 : (int)(t * FPS);
	if (want < frame) {
	    sleep_for(frame / (double)FPS - t);
	    continue;
	}
	frame = want;
	if (frame >= FRAMES)
	    break;
	T = (float)frame / FPS;
	for (int y = 0; y < H; y++) {
	    for (int x = 0; x < W; x++) {
		float r, g, b;
		int sx = x + (int)__builtin_lrintf(1.5f * sinf(T * 1.5f - x * 0.08f));
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
			float glow = ex(-(dsun + SUN_R) * 0.12f);
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
		    float light = ex(-fabsf(x - SUN_X) * 0.06f) * (1 - depth);
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

    put_s("\x1b[0m\x1b[?25h\x1b[?1049l");
    flush_out();
    sys3(SYS_WAITPID, player, 0, 0);
    return 0;
}
