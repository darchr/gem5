#include <stdio.h>
#include <stdlib.h>
#include "../graph_types.h"
#include <fstream>

#ifdef GEM5
#include "gem5/m5ops.h"
#endif

const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
const uint64_t EL_addr = 0x600000000;
const uint64_t VL_addr = 0x800000000;
const uint64_t initalized_addr = 0x200000000;
const uint64_t finished_addr = 0x300000000;
const uint64_t finished_flag = 0x310000000;
const uint64_t activeList_addr = 0x400000000;


int main(int argc, char* argv[]) {

    // Dont use with gem5!!!!
    // Edge* EL = (Edge*)malloc(4096 * sizeof(Edge) * 200);
    // Vertex* VL = (Vertex*)malloc(4096 * sizeof(Vertex) * 2);
#ifdef GEM5
printf("GEM5 is defined\n");
#endif
    uint16_t num_cores = 2;

    if(argc > 1){
        num_cores = atoi(argv[1]);
    }

    Update* messageQueue = (Update*)(buffer_addr); 


    // Your code here
    
    Edge* EL = (Edge*)EL_addr;
    Vertex* VL = (Vertex*)VL_addr;
    uint64_t* initalized = (uint64_t*)initalized_addr;
    uint8_t* finished = (uint8_t*)finished_addr;
    uint8_t* finish_flag = (uint8_t*)finished_flag;
    finish_flag[0] = 0;

    bool is_Weighted = false;
    uint64_t num_nodes = 81305;
    uint64_t EL_start;

    uint16_t initial = 0;

    #ifdef GEM5
        printf("Resetting stats!\n");    
        m5_reset_stats(1, 1<<51);
    #endif
    
    Update initial_update = Update(0, initial);
    *messageQueue = initial_update;

    printf("writing to initalized\n");
    *initalized = num_nodes;
    
    uint64_t done = 0;
    uint64_t counter = 0;
    while(counter < 1000){
        done = finished[0];

        for(int i = 1; i < num_cores; i++){
            // if(counter%5000 == 0){
            //     printf("Finished[%d] = %d, addr = %ld\n", i, finished[i], finished);
            // }
            done &= finished[i];
        }
        
        if(done == 1){
            counter++;
        }
        else{
            counter = 0;
        }
    }

    printf("Writing to finish flag!\n");
    // delaying
    // for(int i = 0; i < 100; i++){
    //     // printf("i = %d\n", i);
    // }
    finish_flag[0] = 1;


    // print whole graph
    // for(int x = 0; x < num_nodes - 15000; x++){
    //  uint64_t x = 18716;
    //     printf("Vertex %d: ", x);
    //     for(int y = 0; y < VL[x].EL_size; y++){
    //         printf("Edge %ld: %ld \n",y,  EL[VL[x].EL_start + y].neighbor);
    //     }
    //     printf("\n");
    // }

    return 0;
}
