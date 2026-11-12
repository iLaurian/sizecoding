#include <stdio.h>
#include <math.h>

#define W 120
#define H 60
#define HORIZON 40

#define SUN_X 85
#define SUN_Y 34
#define SUN_R 6

#define KX 40
#define KY 22

unsigned char img[H][W][3];

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
	d = fminf(d, circle(x, y, KX - i * 0.9f, KY - 12 + i * i * 0.055f, 3.5f - i * 0.075f));
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

int main(void) {
    for (int y = 0; y < H; y++) {
	for (int x = 0; x < W; x++) {
	    float r, g, b;
	    int tip = HORIZON - (int)(hash(x) * 4);
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
		float stalk = hash(x / width + y / 7 * 1000);
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

		if (fabsf(circle(x, y, 18, 72, 25) + 5) < 0.6) {
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
	    put(x, y, r, g, b);
	}
    }

    for (int y = 0; y < H; y += 2) {
	for (int x = 0; x < W; x++) {
	    unsigned char *top = img[y][x], *bot = img[y + 1][x];
	    printf("\x1b[38;2;%d;%d;%dm"
		   "\x1b[48;2;%d;%d;%dm"
		   "\xe2\x96\x80",
		   top[0], top[1], top[2], bot[0], bot[1], bot[2]);
	}
	printf("\x1b[0m\n");
    }
    return 0;
}
