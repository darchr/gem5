#include <cstdint>
#include <sys/mman.h>
#include <iostream>
#include <tuple>

const uint64_t buffer_addr = 0x100000000;

const uint64_t buffer_size = 4096;
using namespace std;


    // uint16_t weight : 16;
    // uint64_t dst_id : 48;
    //uint64_t src_id : 48;
struct Update
{
    uint16_t weight : 16;
    uint64_t dst_id : 48;

    Update(): weight(0), dst_id(0) {}

    Update(uint16_t weight, uint64_t dst_id):
        weight(weight),
        dst_id(dst_id)
    {}
};

int main(int argc, char** argv){

    Update* mapped_area = (Update*)buffer_addr;

    std::cout << "Mapped_area " << (void*)mapped_area << std::endl;

    std::cout << "Doing a write" << std::endl;

    *mapped_area = Update(1,2);

    std::cout << "Doing a write Update(1,2)" << std::endl;

    *mapped_area = Update(4,5);
    std::cout << "Doing a write Update(4,5)" << std::endl;

    std::cout << "Mapped_area " << (void*)mapped_area << std::endl;
    Update tester = *mapped_area;
    std::cout << "Read data:  dst_id: " << tester.dst_id << "  Weight: " << tester.weight << std::endl;
    Update b = *mapped_area;
    // Update c = *mapped_area;
    // Update d = *mapped_area;

    std::cout << " second read: dst_id: "<< b.dst_id << "  weight:" << b.weight << std::endl;
    // std::cout << " second read: dst:"<< c.dst_id << "  weight:" << c.weight << std::endl;
    // std::cout << " third read: dst:"<< d.dst_id << "  weight:" << d.weight << std::endl;

    std::cout << "Mapped_area " << (void*)mapped_area << std::endl;

    std::cout << "got to the end!" << std::endl;

    return 0;
}
