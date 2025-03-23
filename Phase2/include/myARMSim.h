/* myRISCVSim.h  
   Header file for myRISCVSim  
*/ 
#include<bits/stdc++.h>
using namespace std;

void run_RISCVsim();
void reset_proc();
void load_program_memory(const std::string& file_name);
void write_data_memory();
void swi_exit();


// Fetches an instruction from memory and updates the instruction register
void fetch();
// Decodes the instruction, retrieves operands from registers, and determines the execution operation  
void decode();
// Executes the ALU operation as per the decoded instruction  
void execute();
// Handles memory read/write operations  
void mem();
// Writes the final computed result back to the register file  
void write_back();


// Writes an instruction to memory at a specified address  
void write_word(const std::string& address, const std::string& instruction);

