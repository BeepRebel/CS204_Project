
#ifndef STATE_H
#define STATE_H
#include<bits/stdc++.h>
using namespace std;
class State {
    public:
        unsigned int PC;
        string instruction_word;
        int rs1;
        int rs2;
        string operand1;
        string operand2;
        int rd;
        int offset;
        string register_data;
        int memory_address;
        int alu_control_signal;
        vector<int> is_mem; // [no memory operation/load/store, type of load/store]
        bool write_back_signal;
    
        // additional control fields
        bool is_dummy;
        int pc_update;
        bool branch_taken;
    
        int inc_select;
        int pc_select;
        int pc_offset;
        int return_address;
        int next_pc;
    
        bool decode_forwarding_op1;
        bool decode_forwarding_op2;
        string asm_code;
    
        // constructor
        State(int pc = 0);
    };
#endif // STATE_H