#include <stdio.h>
#include <stdlib.h>
#include <deque>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>



int main(int argc, char** argv){

    char data[64];
    int num_bytes = 64;
    char* to_write = (char*)"as"; // needs to be at least 1 byte of data in before calling mmap
    
    char* mapped_area; //could replace char with data type/tuple?

    int buffer_size = 4096;
    int buffer_addr = open("buffer_addr", O_RDWR | O_CREAT, S_IRWXU | S_IRWXO);
    int y = write(buffer_addr, to_write, 2);
    // mapped_area = (char*)mmap((void *)0x1000000, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, buffer_addr, 0);  // Use this line to run locally
    mapped_area = (char*)mmap((void *)0x1000000, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED , buffer_addr, 0);  // MAP_FIXED only for use in gem5!
    printf("Mapped_area: %p\n", mapped_area);


    int x;

    mapped_area[2] = 'o';
    mapped_area[3] = 'r';
    mapped_area[4] = 'o';
    mapped_area[5] = 'r';

 



    // FILE* file_stream = fdopen(buffer_addr, "w+");

    // fwrite((void*)"Goodbye", 1, 8, file_stream); 
    // fclose(file_stream);

    // things to note, order isnt necessarily kept, consistency/coherency will be important.
    // if mapped_area[0] = 'Q'; is moved after the print than x = read(...) wont detect the change
    printf("Mapped Area: %s\n", mapped_area);

    // unsigned int microsecond = 1000;
    // for(int z = 0; z < microsecond; z++){
    //     num_bytes++;
    //     num_bytes-=1;
    // }

    x = read(buffer_addr, data, num_bytes);
    printf("Data: %s\n", data);

}