#include <iostream>

typedef long long int v2df __attribute__ ((vector_size (16)));

int main(int argc, char *argv[])
{
    v2df a = {3, 0};
    v2df b = {0, 7};

    __builtin_ia32_movntdq(&a, b);

    long long int *c = (long long int*)&a;
    long long int *d = (long long int*)&a;
    d += 1;

    if (*c != 0 || *d != 7) {
        std::cout << "Wrong answer!" << std::endl;
        std::cout << "Expected: " << 0 << "\t" << 7 << std::endl;
        std::cout << "Result:   " << *c << "\t" << *d << std::endl;
        return 1;
    }
    std::cout << "Correct execution." << std::endl;
    return 0;

}
