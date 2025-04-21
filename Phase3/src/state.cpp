#include <bits/stdc++.h>
#include "../header/state.h"
using namespace std;

// constructor
State::State(int pc) {
    PC = pc;
    instruction_word = "0x0";
    rs1 = -1;
    rs2 = -1;
    operand1 = "0x00000000";
    operand2 = "0x00000000";
    rd = -1;
    offset = 0;
    register_data = "0x00000000";
    memory_address = 0;
    alu_control_signal = -1;
    is_mem = {-1,-1};
    write_back_signal = false;
    is_dummy = false;
    pc_update = -1;
    branch_taken = false;
    inc_select = 0;
    pc_select = 0;
    pc_offset = 0;
    return_address = -1;
    next_pc = -1;
    decode_forwarding_op1 = false;
    decode_forwarding_op2 = false;
    asm_code = "";
}
