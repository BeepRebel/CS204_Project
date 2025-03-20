/* 

The project is developed as part of CS204: Computer Architecture class Project Phase 2 and Phase 3: Functional Simulator for subset of RISCV Processor

Developer's Names:
Group No:
Developer's Email ids:
Date: 

*/


/* main.cpp 
   Purpose of this file: The file handles the input and output, and
   invokes the simulator
*/

#include <bits/stdc++.h>
#include "myRISCVSim.cpp"

#include "../include/myARMSim.h"
using namespace std;

int main() {

    // Reset the processor
    reset_proc();
    
    // Load the program memory
    load_program_memory("../test/simple_add.mc");
    
    // Run the simulator
    run_RISCVsim();
    
    return 0;
}
