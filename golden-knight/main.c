#include <stdio.h>
#include <math.h>

#define W 120
#define H 60
#define HORIZON 40
#define SUN_X 85
#define SUN_Y 34

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

		float dx = x - 85, dy = y - 34;
		float d = sqrtf(dx * dx + dy * dy);
		if (d < 6) {
		    r = 255; g = 250; b = 220;
		} else {
		    float glow = expf(-d * 0.12f);
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
