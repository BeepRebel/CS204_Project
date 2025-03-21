/* 

The project is developed as part of Computer Architecture class
Project Name: Functional Simulator for subset of RISCV Processor

Developer's Name:
Developer's Email id:
Date: 

*/


/* myRISCVSim.h
   Purpose of this file: header file for myRISCVSim
*/
#include<bits/stdc++.h>
using namespace std;

void run_RISCVsim();
void reset_proc();
void load_program_memory(const std::string& file_name);
void write_data_memory();
void swi_exit();


//reads from the instruction memory and updates the instruction register
void fetch();
//reads the instruction register, reads operand1, operand2 fromo register file, decides the operation to be performed in execute stage
void decode();
//executes the ALU operation based on ALUop
void execute();
//perform the memory operation
void mem();
//writes the results back to register file
void write_back();


//int read_word(char *mem, unsigned int address);
void write_word(const std::string& address, const std::string& instruction);

