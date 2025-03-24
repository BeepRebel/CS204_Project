/* main.cpp  
   Handles input and output operations and invokes the simulator  
*/

#include <bits/stdc++.h>
#include "myRISCVSim.cpp"

#include "../include/myARMSim.h"
using namespace std;

int main() {

    // Initialize processor state  
    reset_proc();
    
    // Load program instructions into memory  
    load_program_memory("../test/bubblesort_iterative.mc");
    
    // Run the simulator
    run_RISCVsim();
    
    return 0;
}
