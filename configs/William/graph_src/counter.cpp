#include <stdio.h>
#include <stdlib.h>
#include "../graph_types.h"
#include <fstream>


// const uint64_t buffer_addr = 0x100000000;
const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue

const uint64_t EL_addr = 0x200000000;
const uint64_t VL_addr = 0x300000000;
const uint64_t initalized_addr = 0x400000000;
const uint64_t finished_addr = 0x500000000;
const uint64_t finished_flag = 0x510000000;



int main(int argc, char* argv[]) {

    // Dont use with gem5!!!!
    // Edge* EL = (Edge*)malloc(4096 * sizeof(Edge) * 200);
    // Vertex* VL = (Vertex*)malloc(4096 * sizeof(Vertex) * 2);

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
    uint64_t num_nodes = 1;
    uint64_t EL_start;

    std::ifstream graph_file;
   
    // graph_file.open("test_graph.txt");
    // graph_file.open("graph.txt");
    graph_file.open("facebook_combined.txt");
    // graph_file.open("facebook_shortened.txt");
    if(is_Weighted){

    }else{
        uint64_t src_id, dst_id, curr_src_id;
        graph_file >> curr_src_id >> dst_id;

        uint64_t index = 0;
        uint32_t EL_size = 1;
        EL_start = index;
        EL[index] = Edge(1, dst_id);
        index++;
        //printf("src_id = %ld, dst_id = %ld\n", curr_src_id, dst_id);


        while(graph_file >> src_id >> dst_id){

            if(curr_src_id != src_id){
                num_nodes++;
                VL[curr_src_id] = Vertex(65535, curr_src_id, EL_start, EL_size, false);
                // add code here to check if there is a gap in the vertex ids
                // if there is a gap, add a vertex with no edges
                if(curr_src_id != src_id - 1){
                    for(uint64_t i = curr_src_id + 1; i < src_id; i++){
                        VL[i] = Vertex(65535, i, 0, 0, false);
                        num_nodes++;
                    }
                }

                curr_src_id = src_id;
                EL_start = index;
                EL[index] = Edge(1, dst_id);

                index++;
                EL_size = 1;
            }
            else{
                EL[index] = Edge(1, dst_id);
                index++;
                EL_size++;
            }
            // num_nodes = max(num_nodes, max(src_id, dst_id));
        }
        
        VL[curr_src_id] = Vertex(65535, curr_src_id, EL_start, EL_size, false);
        num_nodes++;
    }



    uint16_t initial = 0;


    *initalized = num_nodes;
    printf("updated initialized at %ld!\n Writing initial update to %ld\n", initalized, messageQueue);

    Update initial_update = Update(0, initial);
    *messageQueue = initial_update;


    uint64_t done = 0;

    while(done == 0){
        done = finished[0];
        for(int i = 1; i < num_cores; i++){
            done &= finished[i];
        }
    }

    printf("Writing to finish flag!\n");
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
