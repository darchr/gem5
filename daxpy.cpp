#include <cstdio>
#include <random>

#ifdef M5OP
#include "util/m5/m5op.h"

#endif

#ifdef ACCEL
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#endif

typedef struct
{
    double *X;
    double *Y;
    double alpha;
    const int N;
} DaxpyParams;


void daxpy(double *X, double *Y, double alpha, const int N)
{
    // Start of daxpy loop
    for (int i = 0; i < N; ++i)
    {
        Y[i] = alpha * X[i] + Y[i];
    }
    // End of daxpy loop
}

void daxpy_accel(double *X, double *Y, double alpha, const int N)
{
    DaxpyParams params = {X, Y, alpha, N};
    volatile int watch = 0;
    volatile int* watch_addr = &watch;
    volatile DaxpyParams* params_addr = &params;
    asm volatile (
        "mov %0,0x10000000\n"
        "\tmov %1,0x10000000\n"
        :
        : "r"(watch_addr), "r"(params_addr)
        :
    );
    // printf("Entering spin loop\n");
    while (watch != 12); // spin
}

int main()
{
    const int N = 10000;
    double *X = (double*)malloc(N*sizeof(double));
    double *Y = (double*)malloc(N*sizeof(double));
    double alpha = 0.5;

    #ifdef ACCEL
    int fd = open("/dev/daxpy", 0);
    uint64_t *driver = (uint64_t*)0x10000000;
    #endif

    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1, 2);
    for (int i = 0; i < N; ++i)
    {
        X[i] = dis(gen);
        Y[i] = dis(gen);
    }

    #ifdef M5OP
    m5_work_begin(0,0);
    #endif

    #ifdef ACCEL
    daxpy_accel(X, Y, alpha, N);
    #else
    daxpy(X, Y, alpha, N);
    #endif

    #ifdef M5OP
    m5_work_end(0,0);
    #endif

    double sum = 0;
    for (int i = 0; i < N; ++i)
    {
        sum += Y[i];
    }
    printf("%lf\n", sum);
    return 0;
}
