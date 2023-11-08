#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "../graph_types.h"
#include <fstream>
#include <deque>

using namespace std;


// Static VA for EL, VL, MessageQueues, and initilization
const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
const uint64_t EL_addr = 0x200000000;
const uint64_t VL_addr = 0x300000000;
const uint64_t initalized_addr = 0x400000000;
const uint64_t finished_addr = 0x500000000;
const uint64_t finished_flag = 0x510000000;
const uint64_t activeList_addr = 0x600000000; // add 4096 to each active_list addr

const uint64_t buffer_size = 4096;
// 
// Steps for generator:
/*
    Spin on initilized addr until an update is seen
    read the num_vertices from initialized_addr
    based on generator_id figure out which active list belongs to you
    Loop:
        Monitor active list for an update
        Upon seeing an update read in the src_id and find the appropriate place on the EL
        Begin writing messages
    If idle for too long, update finished_Addr based on gen id

*/
int main(int argc, char* argv[]){

    uint64_t generator_id = 0;
    uint16_t active_list_len = uint16_t(buffer_size/sizeof(Vertex));
    uint16_t num_msgQueues = 2;

    if(argc > 1){
    generator_id = atoi(argv[1]);
    printf("generator_id: %ld\n", generator_id);
    }
    else{
        printf("Please enter a consumer id\n");
        return 0;
    }

    if(argc > 2){
        num_msgQueues = atoi(argv[2]);
    }

     uint8_t* done = (uint8_t*)(finished_addr+(2*generator_id));// need to fix this, doesnt account for consumers
    //spin on initialized_addr until an update is seen
    
    uint64_t* initialized = (uint64_t*)initalized_addr;
    while(*initialized == 0){
    }
    uint64_t num_vertices = *initialized;

    Update* messageQueue = (Update*)(buffer_addr); 
    // // identifies address of active list
    Vertex* activeList = (Vertex*)(activeList_addr + (generator_id*buffer_size));

    printf("generator id: %ld, activeList addr: %ld\n", generator_id, activeList);

    Vertex* VL = (Vertex*)VL_addr;

    Edge* EL = (Edge*)EL_addr;

    uint8_t* finish_flag = (uint8_t*)finished_flag;

    bool g_flag = false;
    int src_id, dst_id, weight;

    int index = 0; 
    int empty_cycles =0;
            while(!g_flag){
                // if(!activeList.empty()){
                // printf("checking activelist\n\n");

                if(activeList[index].active == true){
                    // printf("generator id: %d   read active list at index %d\n", generator_id, index);

                    empty_cycles = 0;
                    *done = 0;

                    Vertex curr_update = activeList[index];
                    activeList[index].active = false;

                    index = (index+1)%(active_list_len);   
                    // if (index == 1){                
                    //     printf("Accessed active list EL_size: %d\n", curr_update.EL_size);
                    // }
                    // assume non weigted
                    for(uint64_t i = curr_update.EL_start; i < curr_update.EL_start + curr_update.EL_size; i++){
                        Update temp_up;
                        temp_up.weight = curr_update.dist + EL[i].weight;
                        temp_up.dst_id = EL[i].neighbor;
                        //to figure out which threads queue to update, we take the dst_id divided by (vertices per thread)
                        Update* temp_msgQueue = (Update*)(buffer_addr + (buffer_size*(temp_up.dst_id % num_msgQueues)));
                        *temp_msgQueue = temp_up; // check address?
                        
                    }

                }
                else{
                    empty_cycles++;
                    if (empty_cycles > 5000){
                        *done = 1;
                        g_flag = finish_flag[0];
                        if(g_flag == 1){
                            printf("generator_id: %ld, finished\n", generator_id);
                    }
                    }
                    // do nothing

                }
            }



    printf("generator done!\n");

}