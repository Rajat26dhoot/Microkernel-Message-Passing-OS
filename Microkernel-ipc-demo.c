#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ucontext.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>

/* ---------- Utilities: rdtsc and time helpers ---------- */
static inline uint64_t rdtsc(void) {
    unsigned int hi, lo;
    __asm__ __volatile__("cpuid\n\t" "rdtsc\n\t" : "=a"(lo), "=d"(hi) : "a"(0) : "rbx","rcx");
    return ((uint64_t)hi << 32) | lo;
}

static double ns_from_timespec(const struct timespec *t) {
    return (double)t->tv_sec * 1e9 + (double)t->tv_nsec;
}

/* ---------- Stats ---------- */
static int cmp_u64(const void *a, const void *b) {
    uint64_t aa = *(const uint64_t*)a;
    uint64_t bb = *(const uint64_t*)b;
    if (aa < bb) return -1;
    if (aa > bb) return 1;
    return 0;
}

static void print_stats(const char *label, uint64_t *samples, size_t n, double cycles_to_ns) {
    if (n == 0) return;
    uint64_t sum = 0;
    uint64_t min = UINT64_MAX, max = 0;
    for (size_t i=0;i<n;i++) {
        sum += samples[i];
        if (samples[i] < min) min = samples[i];
        if (samples[i] > max) max = samples[i];
    }
    double mean = (double)sum / (double)n;
    uint64_t *cpy = malloc(n * sizeof(uint64_t));
    memcpy(cpy, samples, n * sizeof(uint64_t));
    qsort(cpy, n, sizeof(uint64_t), cmp_u64);
    double median = cpy[n/2];
    double sd = 0.0;
    for (size_t i=0;i<n;i++) {
        double d = (double)samples[i] - mean;
        sd += d*d;
    }
    sd = sqrt(sd / (double)n);

    printf("--- %s ---\n", label);
    printf("samples: %zu\n", n);
    printf("cycles: mean=%.1f median=%.1f min=%" PRIu64 " max=%" PRIu64 " sd=%.1f\n",
           mean, median, min, max, sd);
    printf("per-switch est. (divide round-trip by 2): mean=%.1f cycles (%.2f us) median=%.1f cycles (%.2f us)\n",
           mean/2.0, (mean/2.0)*cycles_to_ns/1000.0, median/2.0, (median/2.0)*cycles_to_ns/1000.0);
    free(cpy);
}

/* ---------- User-level (ucontext) ping-pong ---------- */
static ucontext_t uctx_main, uctx_ping, uctx_pong;
static uint64_t *u_samples = NULL;
static size_t g_iters = 10000;

static void pong_fn(void) {
    for (size_t i=0;i<g_iters;i++) {
        swapcontext(&uctx_pong, &uctx_ping);
    }
    setcontext(&uctx_main);
}

static void ping_fn(void) {
    for (size_t i=0;i<g_iters;i++) {
        uint64_t t0 = rdtsc();
        swapcontext(&uctx_ping, &uctx_pong);
        uint64_t t1 = rdtsc();
        u_samples[i] = t1 - t0;
    }
    setcontext(&uctx_main);
}

static int run_ucontext_test(size_t iters) {
    g_iters = iters;
    u_samples = calloc(iters, sizeof(uint64_t));
    if (!u_samples) return -1;

    const size_t STACK_SZ = 64*1024;
    void *s1 = malloc(STACK_SZ);
    void *s2 = malloc(STACK_SZ);
    if (!s1 || !s2) return -1;

    getcontext(&uctx_ping);
    uctx_ping.uc_stack.ss_sp = s1;
    uctx_ping.uc_stack.ss_size = STACK_SZ;
    uctx_ping.uc_link = &uctx_main;
    makecontext(&uctx_ping, ping_fn, 0);

    getcontext(&uctx_pong);
    uctx_pong.uc_stack.ss_sp = s2;
    uctx_pong.uc_stack.ss_size = STACK_SZ;
    uctx_pong.uc_link = &uctx_main;
    makecontext(&uctx_pong, pong_fn, 0);

    swapcontext(&uctx_main, &uctx_ping);

    free(s1); free(s2);
    return 0;
}

/* ---------- Process + pipes ping-pong ---------- */
static uint64_t *p_samples = NULL;

static int run_process_pipe_test(size_t iters) {
    p_samples = calloc(iters, sizeof(uint64_t));
    if (!p_samples) return -1;

    int p2c[2];
    int c2p[2];
    if (pipe(p2c) < 0) { perror("pipe"); return -1; }
    if (pipe(c2p) < 0) { perror("pipe"); return -1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        close(p2c[1]); close(c2p[0]);
        uint64_t x;
        ssize_t r;
        for (;;) {
            r = read(p2c[0], &x, sizeof(x));
            if (r <= 0) break;
            ssize_t w = write(c2p[1], &x, sizeof(x));
            if (w != sizeof(x)) break;
        }
        _exit(0);
    }
    close(p2c[0]); close(c2p[1]);
    uint64_t msg = 0x12345678ULL;
    for (size_t i=0;i<iters;i++) {
        uint64_t t0 = rdtsc();
        ssize_t w = write(p2c[1], &msg, sizeof(msg));
        if (w != sizeof(msg)) { perror("write"); break; }
        uint64_t reply;
        ssize_t r = read(c2p[0], &reply, sizeof(reply));
        if (r != sizeof(reply)) { perror("read"); break; }
        uint64_t t1 = rdtsc();
        p_samples[i] = t1 - t0;
    }
    close(p2c[1]); close(c2p[0]);
    int status; waitpid(pid, &status, 0);
    return 0;
}

/* ---------- main ---------- */
int main(int argc, char **argv) {
    size_t iters = 10000;
    int run_u = 1, run_p = 1;
    if (argc >= 2) iters = (size_t)strtoull(argv[1], NULL, 10);
    if (argc >= 3) {
        if (argv[2][0] == 'u') { run_p = 0; }
        else if (argv[2][0] == 'p') { run_u = 0; }
        else if (argv[2][0] == 'a') { }
    }

    printf("Microkernel-like IPC & context-switch microbench\n");
    printf("Iterations: %zu\n", iters);
    printf("Running tests: %s%s%s\n",
       run_u ? "ucontext" : "",
       (run_u && run_p) ? " + " : "",
       run_p ? "process(pipes)" : "");


    struct timespec t0, t1;
    uint64_t c0, c1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    c0 = rdtsc();
    usleep(50000);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    c1 = rdtsc();
    double ns_elapsed = ns_from_timespec(&t1) - ns_from_timespec(&t0);
    double cycles_elapsed = (double)(c1 - c0);
    double cycles_to_ns = ns_elapsed / cycles_elapsed;
    double freq_mhz = cycles_elapsed / (ns_elapsed);

    if (run_u) {
        if (run_ucontext_test(iters) == 0) {
            print_stats("User-level ucontext ping-pong (round-trip)", u_samples, iters, cycles_to_ns);
            free(u_samples);
            u_samples = NULL;
        } else {
            fprintf(stderr, "ucontext test setup failed\n");
        }
    }

    if (run_p) {
        if (run_process_pipe_test(iters) == 0) {
            print_stats("Process+pipe ping-pong (round-trip)", p_samples, iters, cycles_to_ns);
            free(p_samples);
            p_samples = NULL;
        } else {
            fprintf(stderr, "process test setup failed\n");
        }
    }

    printf("\nNotes:\n");
    printf(" - ucontext swapcontext() is a user-space cooperative context switch (no kernel scheduler involvement).\n");
    printf(" - pipe-based round-trip measurements include syscall and scheduler overhead. They are useful to estimate OS-level context switch costs (and syscalls), but note that kernels, pipe buffering, and CPU frequency scaling affect results.\n");
    printf(" - The program reports round-trip cycles. We divide by 2 in the summary to estimate a single direction context switch. This is an approximation.\n");

    return 0;
}
