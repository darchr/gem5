#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include "graph_types.h"
#include <fstream>
#include <deque>

using namespace std;


const uint64_t buffer_addr = 0x100000000;
const uint64_t activeList_addr = 0x100000000 + 4096;


const uint64_t buffer_size = 4096;
// 

int main(int argc, char* argv[]){

    Update* mapped_area = (Update*)buffer_addr;

    Update* messageQueue[1];
    messageQueue[0] = mapped_area;

    Update* activeList = (Update*)activeList_addr;
    int index = 0;
    printf("Mapped_area %p\n", mapped_area);
    printf("messageQueue[0] %p\n", messageQueue[0]);
    bool g_flag = true;
    int num_nodes = 11;
    vector<vector<Edge>> EL(num_nodes);// Edge list
    //deque<pair<uint64_t, uint16_t>> activeList; // list of active nodes
     // keeps track of which threads are done
    vector<Vertex> VL;
    for(int z =0; z < num_nodes; z++){ // initialize VL
        if(z ==0){
            VL.emplace_back(Vertex(10, z)); //= Vertex(0, z);
        }
        else{
            VL.emplace_back(Vertex(1000, z));
            //VL[z] = Vertex(-1, z);
        }
    }

    

    ifstream graph_file;
    graph_file.open("test_graph.txt");
    Update initial;
    initial.weight = 0;
    initial.dst_id = 1;
   
   int thread_active = 0;
   int thr1_done = 0;
   int thr2_done = 0;


    
    int src_id, dst_id, weight;
    while(!graph_file.fail()) {

            graph_file >> src_id >> dst_id >> weight;
            printf("src_id: %d dst_id: %d weight: %d\n", src_id, dst_id, weight);
            //weight = rand() % 32 + 1;

        if(src_id != dst_id){
            EL[src_id].push_back(Edge(weight, dst_id));
        }
    }

    //write a function that prints all elements of EL
    for(int i = 0; i < num_nodes; i++){
        printf("EL[%d]: ", i);
        for(Edge e : EL[i]){
            printf("%ld ", e.neighbor);
        }
        printf("\n");
    }

    // we want two threads, one that reads from the message queue and updates the vertex list
    // and one that sees a vertex update and updates the message queues of the neighbors
    //bool g_flag = true;

    *mapped_area = initial;


    int empty_cycles2 = 0;
    //     while(g_flag){
         while(g_flag){

            // need to decide what happens when the queue is empty, return a really high number? or NULL?
            Update next_update = *mapped_area;
            if (next_update.weight == 65535){
                //queue was empty
                empty_cycles2++;
                if (empty_cycles2 > 10000){
                    //thr1_done = 1;
                    //g_flag = ~(thr1_done && thr2_done);
                    g_flag = false;
                }
            }
            else{
                empty_cycles2 = 0;
                thr1_done = 0;// reset done flag
                if(VL[next_update.dst_id].dist > next_update.weight){
                    VL[next_update.dst_id].dist = next_update.weight;
                    // set active List 
                    uint64_t temp = next_update.dst_id;
                    uint16_t temp2 = next_update.weight;
                    //cout << "dst_id: " << temp << "  new weight: " << temp2 << endl;
                    printf("dst_id: %ld  new weight: %d   Updating activeList\n", temp, temp2);
                    
                    //activeList.push_back(make_pair(temp, temp2));
                    Update temporary;
                    temporary.dst_id = next_update.dst_id;
                    temporary.weight = next_update.weight;

                    activeList[index] = temporary;
                    index++;

                    //this will be done in other thread
                    // for(Edge my_edge : EL[next_update.dst_id]){ // for each edge in neighborhood, this is yellow and pink section of sega pseudocode
                    //     Update temp_up;
                    //     temp_up.weight = next_update.weight + my_edge.weight;
                    //     temp_up.dst_id = my_edge.neighbor;
                    //     temp_up.src_id = next_update.dst_id;
                    //     //to figure out which threads queue to update, we take the dst_id divided by (vertices per thread)
                    //     messageQueue[my_edge.neighbor/vpt] = temp_up; //updates the frontier of the corresponding 
                    // }
                }

            }



         }


    // may need a helper thread to check when its all done
    // auto thread_generator=[&mapped_area, &activeList, &thr1_done, &thr2_done, &g_flag, &VL, &EL](int t_id, int n_thr, int vpt){
    // //     // printf("thread id is: %d\n", t_id);

    //     int empty_cycles2 = 0;
    // //     while(g_flag){
    //      while(g_flag){

    //         // need to decide what happens when the queue is empty, return a really high number? or NULL?
    //         Update next_update = *mapped_area;
    //         if (next_update.weight == 65535){
    //             //queue was empty
    //             empty_cycles2++;
    //             if (empty_cycles2 > 1000){
    //                 thr1_done = 1;
    //                 g_flag = ~(thr1_done && thr2_done);
    //             }
    //         }
    //         else{
    //             empty_cycles2 = 0;
    //             thr1_done = 0;// reset done flag
    //             if(VL[next_update.dst_id].dist > next_update.weight){
    //                 VL[next_update.dst_id].dist = next_update.weight;
    //                 // set active List 
    //                 uint64_t temp = next_update.dst_id;
    //                 uint16_t temp2 = next_update.weight;
    //                 //cout << "dst_id: " << temp << "  new weight: " << temp2 << endl;
    //                 printf("dst_id: %ld  new weight: %d   Updating activeList\n", temp, temp2);
    //                 activeList.push_back(make_pair(temp, temp2));

    //                 //this will be done in other thread
    //                 // for(Edge my_edge : EL[next_update.dst_id]){ // for each edge in neighborhood, this is yellow and pink section of sega pseudocode
    //                 //     Update temp_up;
    //                 //     temp_up.weight = next_update.weight + my_edge.weight;
    //                 //     temp_up.dst_id = my_edge.neighbor;
    //                 //     temp_up.src_id = next_update.dst_id;
    //                 //     //to figure out which threads queue to update, we take the dst_id divided by (vertices per thread)
    //                 //     messageQueue[my_edge.neighbor/vpt] = temp_up; //updates the frontier of the corresponding 
    //                 // }
    //             }

    //         }



    //      }
    
    
    // };


    // auto thread_consumer=[&mapped_area, &activeList, &g_flag, &thr2_done, &VL, &EL](int t_id, int n_thr, int vpt){
    // //     // printf("thread id is: %d\n", t_id);
    //         int empty_cycles =0;
    //         while(g_flag){
    //             if(!activeList.empty()){
    //                 printf("activeList is not empty\n");
    //                 empty_cycles = 0;
    //                 thr2_done = 0;
    //                 pair<uint64_t, uint16_t> curr_update = activeList.front();
    //                 activeList.pop_front();
    //                 for(Edge my_edge : EL[curr_update.first]){ // for each edge in neighborhood, this is yellow and pink section of sega pseudocode
    //                     Update temp_up;
    //                     temp_up.weight = curr_update.second + my_edge.weight;
    //                     temp_up.dst_id = my_edge.neighbor;
    //                     //to figure out which threads queue to update, we take the dst_id divided by (vertices per thread)
    //                     // messageQueue[my_edge.neighbor/vpt] = temp_up; //updates the frontier of the corresponding 
    //                     *mapped_area = temp_up;
    //                 }
    //             }
    //             else{
    //                 empty_cycles++;
    //                 if (empty_cycles > 100){
    //                     thr2_done = 1;
    //                 }
    //                 // do nothing

    //             }
    //         }
    // };


    //}



    //thread stuff 
    // for(int x = 0; x < num_threads; x++){
       
    //     threadVector.push_back(thread(thread_sssp, x, num_threads, num_nodes/num_threads));
    // }
    // thread thread1(thread_generator, 0, 2, 1);
    // thread thread2(thread_consumer, 1, 2, 1);

    // thread1.join();
    // thread2.join(); 
    //     threadVector.push_back(thread(thread_end));

    // for(auto& t: threadVector){
    //     t.join();
    // }
    // this part is just to print the path




    printf(" done!\n");


    for(int i = 0; i < num_nodes; i++){
        printf("Vertex %d: dist: %d\n", i, VL[i].dist);
    }
    graph_file.close();
}