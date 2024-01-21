#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include "../graph_types.h"
#include <fstream>
#include <deque>

using namespace std;

// additional optimization: if size == 0, dont write to active list

// const uint64_t activeList_addr = 0x100000000 + 4096;

// Static VA for EL, VL, MessageQueues, and initilization

// const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
// const uint64_t EL_addr = 0x200000000;
// const uint64_t VL_addr = 0x300000000;
// const uint64_t initalized_addr = 0x400000000;
// const uint64_t finished_addr = 0x500000000;
// const uint64_t finished_flag = 0x510000000;
// const uint64_t activeList_addr = 0x600000000; //+ 4096; // add 4096 to each active_list addr

const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
const uint64_t EL_addr = 0x600000000;
const uint64_t VL_addr = 0x800000000;
const uint64_t initalized_addr = 0x200000000;
const uint64_t finished_addr = 0x300000000;
const uint64_t finished_flag = 0x310000000;
const uint64_t activeList_addr = 0x400000000; //+ 4096; // add 4096 to each active_list addr


const uint64_t buffer_size = 4096;
const uint64_t activeList_size = 65536;

// 
// Steps for consumer:
/*
    Spin on initilized addr until an update is seen
    read the num_vertices from initialized_addr
    based on consumer_id figure out which active list and messageQueue belongs to you
    Loop:
        Monitor messageQueue for an update
        Upon seeing an update read in the update and check if that update is better than vertex
        if it is, update vertex and active list
        
    If idle for too long, update finished_Addr based on consumer id

*/
int main(int argc, char* argv[]){

    uint64_t consumer_id = 0;
    uint32_t active_list_len = uint32_t(activeList_size/sizeof(Vertex)); // could potentially implement active list as pointer to a vector
    // read in command line argument into a uint64_t id;
    if(argc > 1){
    consumer_id = atoi(argv[1]);
    printf("consumer_id: %ld\n", consumer_id);
    }
    else{
        printf("Please enter a consumer id\n");
        return 0;
    }

    uint8_t* done = (uint8_t*)(finished_addr+(2*consumer_id) + 1);
    // printf("consumer_id: %ld, done: %ld\n", consumer_id, done);
    //spin on initialized_addr until an update is seen
    
    uint64_t* initialized = (uint64_t*)initalized_addr;
    // printf("reading from inialized addr: %ld\n", initialized);
    while(*initialized == 0){
    }
    uint64_t num_vertices = *initialized;
    // printf("num_vertices: %ld\n", num_vertices);

    uint32_t num_msg_read = 0;
    uint32_t num_activeList_updates = 0;
        uint32_t num_full_activeList = 0;



    // // identifies address of message queue
    Update* messageQueue = (Update*)(buffer_addr + (consumer_id*buffer_size)); 
    // // identifies address of active list
    Vertex* activeList = (Vertex*)(activeList_addr + (consumer_id*activeList_size));

    Vertex* VL = (Vertex*)VL_addr;

    // Edge* EL = (Edge*)EL_addr;

    uint8_t* finish_flag = (uint8_t*)finished_flag;

    int index = 0;
    
    // printf("messageQueue[0] %p\n", messageQueue[0]);
    bool g_flag = false;

    // we want two threads, one that reads from the message queue and updates the vertex list
    // and one that sees a vertex update and updates the message queues of the neighbors
    //bool g_flag = true;



    uint64_t empty_cycles2 = 0;
    
         while(!g_flag){

            // need to decide what happens when the queue is empty, return a really high number? or NULL?
            Update next_update = *messageQueue;

            if (next_update.weight == 65535){
                //queue was empty
                empty_cycles2++;
                if (empty_cycles2 > 1000){

                    *done = 1;
                    g_flag = finish_flag[0];
                    if(g_flag == 1){
                        printf("consumer_id: %ld, finished\n", consumer_id);
                    }
                }
            }
            else{

                empty_cycles2 = 0;
                *done = 0;// reset done flag
                if(VL[next_update.dst_id].dist > next_update.weight){


                    VL[next_update.dst_id].dist = next_update.weight;
                    // set active List 
                    Vertex temporary = VL[next_update.dst_id];
                    temporary.active = true;

                    // removed for simplicity
                    // if(activeList[index].active == true){
                    //     // printf("consumer_id: %ld, active list full\n", consumer_id);
                    //     num_full_activeList++;
                    //     // printf("Consumer %d spinning on active list\n", consumer_id);
                    //     while(activeList[index].active == true){} // spin until active list is not full
                    //     // printf("Consumer %d done spinning on active list\n", consumer_id);

                    // }
                    // printf("writing to active list Update.dist: %ld, id: %ld, EL_start: %ld, EL_size %ld, active: %d \n", temporary.dist, temporary.id, temporary.EL_start, temporary.EL_size, temporary.active);
                    // can either figure out little endian stuff or only use 64 bits for active list
                    // how would we cut down to 64 bits? only need EL_start, EL_size and dist. EL_start:34 EL_size: 18 dist: 12
                    activeList[index] = temporary;
                    index = (index+1)%(active_list_len);
                    num_activeList_updates++;
                    // printf("consumer_id: %ld, updated vertex %d Updating active List, temporary EL_size = %d, \n", consumer_id, next_update.dst_id, temporary.EL_size);

                }
                num_msg_read++;

            }



         }



    

    //if(consumer_id == 0){ // just to test correctness at a known point
        // for(int i = 0; i < num_vertices; i++){
            int print = 58;
            int prints[6] = {4038, 3927, 3271, 591, 3709, 1524};
            // printf("VL[%d]: dist: %d, id: %d\n", print + consumer_id, VL[print+ consumer_id].dist, VL[print+ consumer_id].id);

            if(consumer_id == 0){
                for(int i = 0; i < 6; i++){
                    printf("VL[%d]: dist: %d, id: %d\n", prints[i], VL[prints[i]].dist, VL[prints[i]].id);
                }
            }
        // }
    //}
    printf(" Consumer ID: %d done! # of messages read: %d, # of updates to activeList: %d, ran into a full active list %d times, activeList index %d\n", consumer_id, num_msg_read, num_activeList_updates, num_full_activeList, index);

    
}