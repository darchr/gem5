#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include "../graph_types.h"
#include <fstream>
#include <deque>

#define BATCH_SIZE 8

using namespace std;

// additional optimization: if size == 0, dont write to active list

// const uint64_t activeList_addr = 0x100000000 + 4096;

// Static Virtual Addresses for EL, VL, MessageQueues, and initilization
// make sure these are mapped with process.map() in gem5 config file
const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
const uint64_t EL_addr = 0x600000000;
const uint64_t VL_addr = 0x2000000000;
const uint64_t initalized_addr = 0x200000000;
const uint64_t finished_addr = 0x300000000;
const uint64_t finished_flag = 0x310000000;
const uint64_t activeList_addr = 0x400000000; //+ 4096; // add 4096 to each active_list addr


const uint64_t buffer_size = 4096;
const uint64_t activeList_size = 65536;

// 
// Steps for consumer:
/*
    Spin on initialized addr until an update is seen
    based on consumer_id figure out which active list and messageQueue belongs to you
    Loop:
        Monitor messageQueue for an update
        Upon seeing an update read in the update and check if that update is better than vertex
            if it is, update vertex and active list
        
    If idle for too long, update done based on consumer id

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

    // writes to done, when consumer is done, once the counter sees all generators  consumers done, it will write to finished_addr, stopping the program
    uint8_t* done = (uint8_t*)(finished_addr+(2*consumer_id) + 1);

    //spin on initialized_addr until an update is seen
    uint64_t* initialized = (uint64_t*)initalized_addr;

    while(*initialized == 0){
    } // wait here until graph is written to memory, then program starts

    // uint64_t num_vertices = *initialized; // deprecated

    uint32_t num_msg_read = 0; // keeps track of the number of messages read by this consumer
    uint32_t num_activeList_updates = 0; // keeps track of how many valid updates were received, causing a write to active list
    uint32_t num_full_activeList = 0; // keeps track of how many times it tried to write to a full activelist



    // // identifies address of message queue this consumer reads from
    Update* messageQueue = (Update*)(buffer_addr + (consumer_id*buffer_size)); 

    // Update_vector* messageQueue = (Update_vector*)(buffer_addr + (consumer_id*buffer_size));

    // // identifies address of active list, this consumer wites to
    Vertex* activeList = (Vertex*)(activeList_addr + (consumer_id*activeList_size));

    // Address of the Vertex List
    Vertex* VL = (Vertex*)VL_addr;

    // Edge* EL = (Edge*)EL_addr;

    uint8_t* finish_flag = (uint8_t*)finished_flag;
    // finish_flag[0] = 0;
    int index = 0;
    
    uint8_t g_flag = 0;

    // we want two threads, one that reads from the message queue and updates the vertex list
    // and one that sees a vertex update and updates the message queues of the neighbors
    //bool g_flag = true;
    printf(" consumer id: %d  message_queue addr: %p, active_list addr = %p, finish_flag = %d\n",consumer_id, messageQueue, activeList, finish_flag[0]);



    // uint64_t empty_cycles2 = 0;

    
    
        // Loop continues until all generators and consumers are done
         while(g_flag != 1){

            // Update retrieved_updates[BATCH_SIZE]; // multiple memory access
            // // need to decide what happens when the queue is empty, return a really high number? or NULL?
            // for(int i =0; i < BATCH_SIZE; i++){ // multiple memory access
            //     retrieved_updates[i] = *(messageQueue+(i*64)); // multiple memory access
            // } // multiple memory access

            // Update_vector retrieved_updates = *messageQueue; // multiple memory access


            Update next_update = *messageQueue; // slowdown is that only one element is accessed at a time. Might speed up to batch
          
            // printf("next_update_addr: %p\n", &next_update);
            
            // Update next_update = retrieved_updates[j]; // multiple memory access

            // message queue will return an update with the max_weight(65535) if the queue is empty
            if (next_update.weight == 65535){
                //queue was empty
                // empty_cycles2++;
                // if (empty_cycles2 > 1000){ // if the queue is empty for too long, assume this consumer is done

                    // removing while trying messageQueue writing to done
                    // *done = 1;
                    g_flag = finish_flag[0];
                    if(g_flag != 0){
                        printf("consumer %d is done, finish_flag[0] = %d\n", consumer_id, finish_flag[0]);
                    }
                    // 
                    //  break; // multiple memory access
                // }
            }
            else{
                // read a valid update from the message queue
                // removing while trying messageQueue writing to done

                // empty_cycles2 = 0;
                // *done = 0;// reset done flag

                // check if update has a better weight than the current vertex
                if(VL[next_update.dst_id].dist > next_update.weight){

                    // update vertex
                    VL[next_update.dst_id].dist = next_update.weight;
                    
                    // set active List 
                    Vertex temporary = VL[next_update.dst_id];
                    // printf("address of temporary: %p\n", &temporary);
                    temporary.active = true;

                    

                    // can either figure out little endian stuff or only use 64 bits for active list
                    // how would we cut down to 64 bits? only need EL_start, EL_size and dist. EL_start:34 EL_size: 15 dist: 15?
                    activeList[index] = temporary; // write to active list
                    
                    // num_activeList_updates++;

                }
                // num_msg_read++;

            // } // multiple memory access

            }



         }



    


            int prints[6] = {4038, 3927, 3271, 591, 3709, 1524}; // random vALUES TO CHECK CORRECTNESS

            if(consumer_id == 0){
                for(int i = 0; i < 6; i++){
                    printf("VL[%d]: dist: %d, id: %d\n", prints[i], VL[prints[i]].dist, VL[prints[i]].id);
                }
            }
                        printf("consumer %d is done, finish_flag[0] = %d\n", consumer_id, finish_flag[0]);

    printf(" Consumer ID: %d done! # of messages read: %d, # of updates to activeList: %d, ran into a full active list %d times, activeList index %d\n", consumer_id, num_msg_read, num_activeList_updates, num_full_activeList, index);

    
}