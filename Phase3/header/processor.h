#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <bits/stdc++.h>
#include "../header/state.h"
#include "../header/btb.h"
using namespace std;

class Processor {
public:
    // Memory and Registers
    unordered_map<int, string> MEM;
    vector<string> R;
    
    // Control Flags and States
    bool pipelining_enabled;
    bool terminate;
    int next_PC;
    int inc_select;
    int pc_select;
    int return_address;
    int pc_offset;
    bool all_dummy;
    
    // Instruction Counts
    unordered_map<int, string> get_code;
    int count_total_inst;
    int count_alu_inst;
    int count_mem_inst;
    int count_control_inst;
    int count_branch_mispredictions;

    // Constructor
    Processor(string file_name);
    
    // Reset Processor state
    void reset();
    void reset(State *state);

    // Load program into memory (method to be defined)
    void load_program_memory(const string &file_name);

    // Fetch instruction based on current PC (method to be defined)
    void write_word(const std::string &address, const std::string &instruction);

    void write_data_memory();

    void IAG(State *state);

    void fetch(State *state, BTB *btb);

    // Decode instruction (method to be defined)
    tuple<bool, int, bool, int> decode(State *state, BTB *btb);

    string formatInstruction(const std::string& instr, int rd, int rs1, int rs2);
    // Execute instruction (method to be defined)
    void execute(State *state);
    
    // Handle memory access (method to be defined)
    void mem(State *state);

    // Write-back (method to be defined)
    void writeBack(State *state);

    // Simulation loop (method to be defined)
    void swi_exit();
};

#endif // PROCESSOR_H
