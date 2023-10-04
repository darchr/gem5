#include <cstdint>
#include <sys/mman.h>
#include <iostream>

const uint64_t buffer_addr = 0x1000000;
const uint64_t buffer_size = 4096;

int main(int argc, char** argv){

    uint8_t *mapped_area = (uint8_t*)buffer_addr;

    std::cout << "Mapped_area " << (void*)mapped_area << std::endl;

    std::cout << "Doing a read" << std::endl;

    int a = *mapped_area;

    std::cout << "Read " << a << std::endl;

    std::cout << "Doing a write" << std::endl;

    *mapped_area = 42;


    std::cout << "got to the end!" << std::endl;

    return 0;
}
