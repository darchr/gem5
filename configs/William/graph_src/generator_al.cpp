#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "../graph_types.h"
#include <fstream>
#include <deque>
#include <string>
#include <iostream>

using namespace std;

// Static VA for EL, VL, MessageQueues, and initilization

const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
const uint64_t EL_addr = 0x600000000;
const uint64_t VL_addr = 0x2000000000;
const uint64_t initalized_addr = 0x200000000;
const uint64_t finished_addr = 0x300000000;
const uint64_t finished_flag = 0x310000000;
const uint64_t activeList_addr = 0x400000000;

const uint64_t buffer_size = 4096;
const uint64_t activeList_size = 65536;
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
    uint32_t active_list_len = uint32_t(activeList_size/sizeof(Vertex));
    uint16_t num_msgQueues = 2;

    uint32_t num_msgs_sent = 0;
    uint32_t num_activeList_updates = 0; // number of times active list was updated
    // uint32_t num_full_activeList = 0;



    if(argc > 1){
        generator_id = atoi(argv[1]);
        printf("generator_id: %ld\n", generator_id);
    }
    else{
        printf("Please enter a generator id\n");
        return 0;
    }

    if(argc > 2){
        num_msgQueues = atoi(argv[2]);
    }


    char filename[20];
    sprintf(filename, "generator_%d.txt", generator_id);
    ofstream out_file;
    out_file.open(filename);
    //  uint8_t* done = (uint8_t*)(finished_addr+(2*generator_id));// need to fix this, doesnt account for consumers
    //spin on initialized_addr until an update is seen
    
    uint64_t* initialized = (uint64_t*)initalized_addr;

    while(*initialized == 0){
    }
    // uint64_t num_vertices = *initialized;

    Update* messageQueue = (Update*)(buffer_addr); 
    // // identifies address of active list
    Vertex* activeList = (Vertex*)(activeList_addr + (generator_id*activeList_size));

    // Vertex* VL = (Vertex*)VL_addr;

    Edge* EL = (Edge*)EL_addr;

    uint8_t* finish_flag = (uint8_t*)finished_flag;

    uint8_t g_flag = 0;

    string to_print;
    // int src_id, dst_id, weight;
    // bool printed = false;
    int index = 0; 
    // int empty_cycles = 0;

        printf(" generator id: %d  active_list addr = %p, num_queues = %d, other pointers: %lu, %lu \n",generator_id, activeList, num_msgQueues, &finish_flag, &initialized);

            int temp_print = 0;
            while(g_flag == 0){
                temp_print++;
                if(generator_id == 0 && temp_print > 50){
                    temp_print = 0;
                    printf("generator loop\n");
                }


                Vertex curr_update = activeList[index];

                // if(activeList[index].active == true){
                if(curr_update.active == true){

                    // empty_cycles = 0;
                    // *done = 0;
                    // Vertex curr_update = activeList[index];
                    // activeList[index].active = false;

                    // Unecessary when using AL hardware
                    // index = (index+1)%(active_list_len); 
                    // if(generator_id == 3){  
                    //     printf("curr update id: %d\n", curr_update.id);
                    // }
                    for(uint64_t i = curr_update.EL_start; i < curr_update.EL_start + curr_update.EL_size; i++){

                        Update* temp_up = new Update(0,0);
                        temp_up->weight = curr_update.dist + EL[i].weight;
                        temp_up->dst_id = EL[i].neighbor;
                        //to figure out which threads queue to update, we take the dst_id divided by (vertices per thread)
                        Update* temp_msgQueue = (Update*)(buffer_addr + (buffer_size*(temp_up->dst_id % num_msgQueues)));

                        *temp_msgQueue = *temp_up; // check address?
                        
                            // printf("temp_msgQueue: %p, temp_up: %p\n", temp_msgQueue, temp_up);
                        out_file << "temp_msgQueue: " << temp_msgQueue << ", temp_up: " << temp_up << " EL index = " << i << "  curr update id: " << curr_update.id << endl;
                        
                        // num_msgs_sent++;
                    }

                    // num_activeList_updates++;

                }
                else{
                    // empty_cycles++;
                    // if (empty_cycles > 1000){
                        // *done = 1;
                        g_flag = finish_flag[0];
                        // if (g_flag!=0)
                        // {
                            printf("generator %d, finish_flag[0] = %d\n", generator_id, finish_flag[0]);
                        // }
                        

                        if(g_flag == 1){
                            printf("generator %d is done\n", generator_id);
                        }
                    // }
                    // do nothing

                }
            }
                        printf("generator %d is done, finish_flag[0] = %d\n", generator_id, finish_flag[0]);

    printf("generator id: %d done!  # of messages sent: %d, # of activeList reads: %d, activelist index = %d\n", generator_id, num_msgs_sent, num_activeList_updates, index);

}