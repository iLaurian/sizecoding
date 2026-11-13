#ifndef SYS_H
#define SYS_H

__attribute__((unused)) static int sys3(int n, int a, int b, int c) {
    int r;
    asm volatile ("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return r;
}

#define SYS_EXIT          1
#define SYS_FORK          2
#define SYS_WRITE         4
#define SYS_WAITPID       7
#define SYS_NANOSLEEP     162
#define SYS_CLOCK_GETTIME 265

__attribute__((unused)) static void write_all(int fd, const void *data, int n) {
    const char *p = data;
    while (n > 0) {
	int w = sys3(SYS_WRITE, fd, (int)p, n);
	if (w <= 0)
	    return;
	p += w;
	n -= w;
    }
}

struct timeval32 {
    int sec;
    int nsec;
};

static int start_sec;

__attribute__((unused)) static float elapsed(void) {
    struct timeval32 t;
    sys3(SYS_CLOCK_GETTIME, 1, (int)&t, 0);
    if (!start_sec)
	start_sec = t.sec;
    return (t.sec - start_sec) + t.nsec * 1e-9f;
}

__attribute__((unused)) static void sleep_for(float seconds) {
    struct timeval32 t;
    t.sec = (int)seconds;
    t.nsec = (int)((seconds - t.sec) * 1e9);
    sys3(SYS_NANOSLEEP, (int)&t, 0, 0);
}

__attribute__((unused)) static float ex2(float x) {
    float n = __builtin_rintf(x);
    float f = x - n;
    float r;
    asm ("f2xm1" : "=t"(r) : "0"(f));
    r += 1;
    asm ("fscale" : "=t"(r) : "0"(r), "u"(n));
    return r;
}

__attribute__((unused)) static float ex(float x) {
    float r;
    asm ("fldl2e\n\t" "fmulp %%st, %%st(1)" : "=t"(r) : "0"(x));
    return ex2(r);
}

int main(void);

__attribute__((force_align_arg_pointer, noreturn))
void _start(void) {
    sys3(SYS_EXIT, main(), 0, 0);
    __builtin_unreachable();
}

#endif
