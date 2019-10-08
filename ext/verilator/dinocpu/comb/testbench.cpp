#include <verilated.h>

#include "VTop.h"

int main(int argc, char** argv, char** env){
    Verilated::commandArgs(argc, argv);
    VTop* top = new VTop;
    //reset
    top->reset = 1;
    top->clock = 0;
    top->eval();
    top->clock = 1;
    top->eval();
    top->reset = 0;

    while (!Verilated::gotFinish()){
        top->clock = 0;
        top->eval();

        top->clock = 1;
        top->eval();
    }
    delete top;
    exit(0);
}
