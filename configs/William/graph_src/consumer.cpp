#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include "../graph_types.h"
#include <fstream>
#include <deque>

using namespace std;



// const uint64_t activeList_addr = 0x100000000 + 4096;

// Static VA for EL, VL, MessageQueues, and initilization
const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
const uint64_t EL_addr = 0x200000000;
const uint64_t VL_addr = 0x300000000;
const uint64_t initalized_addr = 0x400000000;
const uint64_t finished_addr = 0x500000000;
const uint64_t finished_flag = 0x510000000;
const uint64_t activeList_addr = 0x600000000; //+ 4096; // add 4096 to each active_list addr


const uint64_t buffer_size = 4096;
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
    uint16_t active_list_len = uint16_t(buffer_size/sizeof(Vertex)); // could potentially implement active list as pointer to a vector
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
    //spin on initialized_addr until an update is seen
    
    uint64_t* initialized = (uint64_t*)initalized_addr;
    printf("reading from inialized addr: %ld\n", initialized);
    while(*initialized == 0){
    }
    uint64_t num_vertices = *initialized;
    printf("num_vertices: %ld\n", num_vertices);


    // // identifies address of message queue
    Update* messageQueue = (Update*)(buffer_addr + (consumer_id*buffer_size)); 
    // // identifies address of active list
    Vertex* activeList = (Vertex*)(activeList_addr + (consumer_id*buffer_size));

    Vertex* VL = (Vertex*)VL_addr;

    Edge* EL = (Edge*)EL_addr;

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
                if (empty_cycles2 > 5000){

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

                    activeList[index] = temporary;
                    index = (index+1)%(active_list_len);

                }

            }



         }



    printf(" Consumer ID: %d done!\n", consumer_id);

    if(consumer_id == 0){ // just to test correctness at a known point
        // for(int i = 0; i < num_vertices; i++){
            printf("VL[%d]: dist: %d, id: %d\n", 1035, VL[1035].dist, VL[1035].id);
        // }
    }


    
}