#include <bits/stdc++.h>
#include "../header/processor.h"
#include "../header/utils.h"
using namespace std;

const unordered_map<int, unordered_map<string, tuple<string, int, string>>> instruction_map = {
    // R-type instructions (opcode 0110011)
    {0b0110011, {
        {"0_0", {"add", 2, "R"}},
        {"0_32", {"sub", 8, "R"}},       
        {"7_0", {"and", 1, "R"}},
        {"6_0", {"or", 3, "R"}},
        {"1_0", {"sll", 4, "R"}},
        {"2_0", {"slt", 5, "R"}},
        {"5_32", {"sra", 6, "R"}},
        {"5_0", {"srl", 7, "R"}},
        {"4_0", {"xor", 9, "R"}},
        {"0_1", {"mul", 10, "R"}},
        {"4_1", {"div", 11, "R"}},
        {"6_1", {"rem", 12, "R"}}
    }},
    
    // I-type ALU instructions (opcode 0010011)
    {0b0010011, {
        {"0_-1", {"addi", 14, "I"}},
        {"7_-1", {"andi", 13, "I"}},
        {"6_-1", {"ori", 15, "I"}}
    }},
    
    // I-type Load instructions (opcode 0000011)
    {0b0000011, {
        {"0_-1", {"lb", 16, "I"}},
        {"1_-1", {"lh", 17, "I"}},
        {"2_-1", {"lw", 18, "I"}},
        {"3_-1", {"ld", 30, "I"}}
    }},
    
    // JALR instruction (opcode 1100111)
    {0b1100111, {
        {"0_-1", {"jalr", 19, "I"}}
    }},
    
    // S-type instructions (opcode 0100011)
    {0b0100011, {
        {"0_-1", {"sb", 20, "S"}},
        {"1_-1", {"sh", 22, "S"}},
        {"2_-1", {"sw", 21, "S"}},
        {"3_-1", {"sd", 31, "S"}}
    }},
    
    // SB-type branch instructions (opcode 1100011)
    {0b1100011, {
        {"0_-1", {"beq", 23, "SB"}},
        {"1_-1", {"bne", 24, "SB"}},
        {"5_-1", {"bge", 25, "SB"}},
        {"4_-1", {"blt", 26, "SB"}}
    }},
    
    // U-type instructions
    {0b0010111, {
        {"-1_-1", {"auipc", 27, "U"}}
    }},
    {0b0110111, {
        {"-1_-1", {"lui", 28, "U"}}
    }},
    
    // UJ-type instructions
    {0b1101111, {
        {"-1_-1", {"jal", 29, "UJ"}}
    }}
};

    
Processor::Processor(string file_name) {
    R = vector<string>(32, "0x00000000");
    R[2] = "0x7FFFFFF0";
    R[3] = "0x10000000";
    load_program_memory(file_name);
    pipelining_enabled = false;
    terminate = false;
    next_PC = 0;
    inc_select = 0;
    pc_select = 0;
    return_address = -1;
    pc_offset = 0;
    all_dummy = false;
    count_total_inst = 0;
    count_alu_inst = 0;
    count_mem_inst = 0;
    count_control_inst = 0;
    count_branch_mispredictions = 0;
}

void Processor::reset() {
    inc_select = 0;
    pc_select = 0;
    pc_offset = 0;
    return_address = 0;
}

void Processor::reset(State *state) {
    state->inc_select = 0;
    state->pc_select = 0;
    state->pc_offset = 0;
    state->return_address = 0;
}

// load program from memory file
void Processor::load_program_memory(const string &file_name)
{
    ifstream infile(file_name);

    // check if the file is open, otherwise print an error and exit
    if (!infile)
    {
        cerr << "ERROR: cannot open input file" << endl;
        exit(1);
    }

    string address, instr;

    // read each line and extract address and instruction
    while (infile >> address >> instr)
    {
        write_word(address, instr); // store the instruction in memory
    }

    infile.close(); // close the file after reading it.
}

// Memory write
void Processor::write_word(const std::string &address, const std::string &instruction)
{
    int idx = std::stoi(address.substr(2), nullptr, 16);
    MEM[idx] = instruction.substr(8, 2);
    MEM[idx + 1] = instruction.substr(6, 2);
    MEM[idx + 2] = instruction.substr(4, 2);
    MEM[idx + 3] = instruction.substr(2, 2);
}

// Write memory contents to output files
void Processor::write_data_memory()
{
    ofstream data_out("../output/memory.mc");
    if (!data_out.is_open())
    {
        cout << "ERROR: Error opening memory file" << endl;
        return;
    }

    // data memory range from 268435456 to 268468221 (0x10000000 to 0x1000FFFD)
    for (unsigned int i = 268435456; i < 268468221; i += 4)
    {
        if (MEM.find(i) != MEM.end() || MEM.find(i + 1) != MEM.end() ||
            MEM.find(i + 2) != MEM.end() || MEM.find(i + 3) != MEM.end())
        {
            // get value for each byte (default to "00" if not present)
            string byte0 = MEM.count(i) ? MEM[i] : "00";
            string byte1 = MEM.count(i + 1) ? MEM[i + 1] : "00";
            string byte2 = MEM.count(i + 2) ? MEM[i + 2] : "00";
            string byte3 = MEM.count(i + 3) ? MEM[i + 3] : "00";

            data_out << "0x" << hex << i << " 0x" << byte3 << byte2 << byte1 << byte0 << dec << endl;
        }
    }

    data_out.close();

    // write register contents to registerFile.mc
    ofstream reg_out("../output/registerFile.mc");
    if (!reg_out.is_open()){
        cout << "ERROR: Error opening registerFile" << endl;
        return;
    }

    // iterate through all 32 registers and write their values
    for (int i = 0; i < 32; i++)
    {
        string reg_value = R[i];
        reg_value = reg_value.substr(0, 2) + string(10 - reg_value.length(), '0') + reg_value.substr(2);    
        reg_out << "x" << dec << i << " " << reg_value << endl;
    }
}

void Processor::IAG(State *state) {
    if (state->pc_select) {
        this->next_PC = state->return_address;
    } else if (state->inc_select) {
        this->next_PC += state->pc_offset;
    } else {
        this->next_PC += 4;
    }

    state->pc_select = 0;
    state->inc_select = 0;
}

void Processor::fetch(State *state, BTB *btb) {
    // if the state is a dummy, skip processing
    if (state->is_dummy) return;

    // if all states are dummy, mark this one as dummy and return
    if (all_dummy) {
        state->is_dummy = true;
        return;
    }

    // construct 32-bit instruction from 4 bytes in memory (little-endian)
    string byte0 = MEM.count(state->PC) ? MEM[state->PC] : "00";
    string byte1 = MEM.count(state->PC + 1) ? MEM[state->PC + 1] : "00";
    string byte2 = MEM.count(state->PC + 2) ? MEM[state->PC + 2] : "00";
    string byte3 = MEM.count(state->PC + 3) ? MEM[state->PC + 3] : "00";

    // combine bytes to form the instruction word
    state->instruction_word = "0x" + byte3 + byte2 + byte1 + byte0;


    // if pipelining is disabled, skip prediction
    if (!pipelining_enabled) return;

    // branch prediction using BTB
    if (btb->find(state->PC)) {
        state->branch_taken = btb->predict(state->PC);
        state->next_pc = state->branch_taken ? btb->getTarget(state->PC) : state->PC + 4;
    }
}

tuple<bool, int, bool, int> Processor::decode(State *state, BTB *btb) {
    if (state->is_dummy) {
        return {false, 0, false, 0};
    }
    
    if (state->instruction_word == "0x00000000") {
        terminate = true;
        state->is_dummy = true;
        all_dummy = true;
        return {false, 0, false, 0};
    }

    string hex_part = state->instruction_word.substr(2); // Remove '0x' prefix
    string bin_instruction = "";

    for (char c : hex_part) {
        bin_instruction += bitset<4>(stoul(string(1, c), nullptr, 16)).to_string();
    }
    while (bin_instruction.length() < 32)
    {
        bin_instruction = "0" + bin_instruction;
    }

    int opcode = stoi(bin_instruction.substr(25, 7), nullptr, 2);
    int func3 = -1;
    int func7 = -1;
    
    if (bin_instruction.length() >= 20) {
        func3 = stoi(bin_instruction.substr(17, 3), nullptr, 2);
    }
    
    if (bin_instruction.length() >= 7) {
        func7 = stoi(bin_instruction.substr(0, 7), nullptr, 2);
    }

    string op_type;
    state->alu_control_signal = -1;
    state->is_mem = {-1, -1};
    
    // Create key for function lookup
    stringstream keyBuilder;
    keyBuilder << func3 << "_" << func7;
    string funcKey = keyBuilder.str();
    string wildcardKey1 = to_string(func3) + "_-1";  // Wildcard for func7
    string wildcardKey2 = "-1_-1";                  // Wildcard for both

    // Lookup instruction in the dictionary
    auto opcode_it = instruction_map.find(opcode);
    if (opcode_it != instruction_map.end()) {
        bool found = false;
        
        // First try exact match
        auto func_it = opcode_it->second.find(funcKey);
        
        // If not found, try with wildcard func7
        if (func_it == opcode_it->second.end()) {
            func_it = opcode_it->second.find(wildcardKey1);
            
            // If still not found, try with wildcard func3 and func7
            if (func_it == opcode_it->second.end()) {
                func_it = opcode_it->second.find(wildcardKey2);
            }
        }
        
        if (func_it != opcode_it->second.end()) {
            state->alu_control_signal = get<1>(func_it->second);
            op_type = get<2>(func_it->second);
            found = true;
        }
        
        if (!found) {
            cout << "ERROR: Invalid machine code" << endl;
            swi_exit();
        }
    } else {
        cout << "ERROR: Invalid machine code" << endl;
        swi_exit();
    }

    if (op_type == "R") {
        state->rs2 = stoi((bin_instruction.substr(7, 5)), nullptr, 2);
        state->rs1 = stoi((bin_instruction.substr(12, 5)), nullptr, 2);
        state->rd  = stoi((bin_instruction.substr(20, 5)), nullptr, 2);
        state->operand1 = R[state->rs1];
        state->operand2 = R[state->rs2];
        state->write_back_signal = true;
    } else if (op_type == "I") {
        state->rs1 = stoi((bin_instruction.substr(12, 5)), nullptr, 2);
        state->rd  = stoi((bin_instruction.substr(20, 5)), nullptr, 2);
        string imm = bin_instruction.substr(0, 12);
        if (!state->decode_forwarding_op1)
            state->operand1 = R[state->rs1];
        state->operand2 = imm;
        state->write_back_signal = true;
    } else if (op_type == "S") {
        state->rs2 = stoi((bin_instruction.substr(7, 5)), nullptr, 2);
        state->rs1 = stoi((bin_instruction.substr(12, 5)), nullptr, 2);
        std::string imm = bin_instruction.substr(0, 7) + bin_instruction.substr(20, 5);
        if (!state->decode_forwarding_op1)
            state->operand1 = R[state->rs1];
        state->operand2 = imm;
        state->register_data = R[state->rs2];
        state->write_back_signal = false;
    } else if (op_type == "SB") {
        state->rs2 = stoi((bin_instruction.substr(7, 5)), nullptr, 2);
        state->rs1 = stoi((bin_instruction.substr(12, 5)), nullptr, 2);
        if (!state->decode_forwarding_op1)
            state->operand1 = R[state->rs1];
        if (!state->decode_forwarding_op2)
            state->operand2 = R[state->rs2];
        string imm = bin_instruction.substr(0, 1) + bin_instruction.substr(24, 1) +
                            bin_instruction.substr(1, 6) + bin_instruction.substr(20, 4) + "0";
        state->offset = nint(imm,2,imm.length());
        state->write_back_signal = false;
    } else if (op_type == "U") {

        state->rd = stoi((bin_instruction.substr(20, 5)), nullptr, 2);
        string imm = bin_instruction.substr(0, 20) + string(12, '0');
        state->operand2 = imm;
        state->write_back_signal = true;

    } else if (op_type == "UJ") {
        state->rd = stoi((bin_instruction.substr(20, 5)), nullptr, 2);
        string imm = bin_instruction.substr(0, 1) + bin_instruction.substr(12, 8) +
                            bin_instruction.substr(11, 1) + bin_instruction.substr(1, 10) + "0";
        state->offset = nint(imm,2,imm.length());
        state->write_back_signal = true;
    } else {
        std::cerr << "ERROR: Unidentifiable machine code!\n";
        swi_exit();
    }

    if (this->pipelining_enabled) {
        vector<int> branch_ins = {23, 24, 25, 26, 29, 19};
        bool entering = false;

        if (std::find(branch_ins.begin(), branch_ins.end(), state->alu_control_signal) == branch_ins.end()) {
            return {false, 0, entering, 0};
        } else {
            this->execute(state);
            this->next_PC = state->PC;
            this->IAG(state);
            int orig_pc = this->next_PC;

            if (btb->find(state->PC) && orig_pc != state->next_pc) {
                this->count_branch_mispredictions++;
            }

            if (!btb->find(state->PC)) {
                state->inc_select = this->inc_select;
                state->pc_select = this->pc_select;
                state->pc_offset = this->pc_offset;
                state->return_address = this->return_address;
                this->next_PC = state->PC;
                this->IAG(state);
                state->pc_update = this->next_PC;
                if (state->alu_control_signal == 19 || state->alu_control_signal == 29)
                    btb->enter(true, state->PC, state->pc_update);
                else
                    btb->enter(false, state->PC, state->pc_update);

                reset();
                reset(state);
                entering = true;
            }

            if (orig_pc != state->next_pc)
                return {true, orig_pc, entering, 1};
            else
                return {false, 0, entering, 3}; // 0: no pred, 1: wrong, 3: correct
        }
    }
    return {false, 0, false, 0};
}

string Processor::formatInstruction(const std::string& instr, int rd, int rs1, int rs2) {
    return instr + " x" + std::to_string(rd) + ", x" + std::to_string(rs1) + ", x" + std::to_string(rs2);
}

void Processor::execute(State *state) {
    if (state->is_dummy) {
        return;
    }

    if (state->alu_control_signal == 2) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        state->register_data = nhex(op1+op2);
        state->asm_code = formatInstruction("add", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 8) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        state->register_data = nhex(op1-op2);
        state->asm_code = formatInstruction("sub", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 1) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        state->register_data = nhex(op1&op2);
        state->asm_code = formatInstruction("and", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 3) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        state->register_data = nhex(op1 | op2);
        state->asm_code = formatInstruction("or", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 4) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        if (op1 < 0) {
            cout << "ERROR: Shift by negative!\n";
            swi_exit();
        }
        else {
            state->register_data = nhex(op1 << op2);
        }
        state->asm_code = formatInstruction("sll", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 5) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        if (op1 < op2) {
            state->register_data = "0x00000001";
        }
        else {
            state->register_data = "0x00000000";
        }
        state->asm_code = formatInstruction("slt", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 6) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        int result = op1 >> op2;
        string bin_value = std::bitset<32>(result).to_string();
    
        // Check if the original value was negative (check the MSB)
        if ((op1 & 0x80000000) != 0) {
        // Fill with 1s for sign extension
            for (int i = 0; i < op2; i++) {
            bin_value = '1' + bin_value;
            }
        }
        state->register_data = stoi(bin_value, nullptr, 2);
        state->asm_code = formatInstruction("sra", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 7) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        if (op2 < 0) {
            cout << "ERROR: Shift by negative!\n";
            swi_exit();
        }
        else {
            state->register_data = nhex(op1 >> op2);
        }
        state->asm_code = formatInstruction("srl", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 9) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        state->register_data = nhex(op1 ^ op2);
        state->asm_code = formatInstruction("xor", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 10) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        state->register_data = nhex(op1*op2);
        state->asm_code = formatInstruction("mul", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 11) {
        if (nint(state->operand2, 16) == 0) {
            std::cout << "ERROR: Division by zero!\n";
            exit(1);
        }
        else {
            int op1 = nint(state->operand1, 16);
            int op2 = nint(state->operand2, 16);
            state->register_data = nhex(op1/op2);
        }
        state->asm_code = formatInstruction("div", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 12) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        state->register_data = nhex(op1%op2);
        state->asm_code = formatInstruction("rem", state->rd, state->rs1, state->rs2);
    }
    else if (state->alu_control_signal == 14) {
        state->register_data = nhex(stoi(state->operand1, nullptr, 16) + nint(state->operand2, 2, state->operand2.length()));
        state->asm_code = formatInstruction("addi", state->rd, state->rs1, stoi(state->operand2,nullptr,2));
    }
    else if (state->alu_control_signal == 13) {
        state->register_data = nhex(stoi(state->operand1, nullptr, 16) & nint(state->operand2, 2, state->operand2.length()));
        state->asm_code = formatInstruction("andi", state->rd, state->rs1, stoi(state->operand2,nullptr,2));
    }
    else if (state->alu_control_signal == 15) {
        state->register_data = nhex(stoi(state->operand1, nullptr, 16) | nint(state->operand2, 2, state->operand2.length()));
        state->asm_code = formatInstruction("ori", state->rd, state->rs1, stoi(state->operand2,nullptr,2));
    }
    else if (state->alu_control_signal == 16) {

        state->memory_address = stoi(state->operand1, nullptr, 16) + nint(state->operand2.substr(2), 2, state->operand2.length());
        state->is_mem = {0, 0};
        state->asm_code = "lw x" + std::to_string(state->rd) + ' ' + to_string(nint(state->operand2, 2, state->operand2.length())) + " (x";
    }
    else if (state->alu_control_signal == 17) {
        state->memory_address = stoi(state->operand1, nullptr, 16) + nint(state->operand2, 2, state->operand2.length());
        state->is_mem = {0, 0};
        state->asm_code = "lh x" + std::to_string(state->rd) + ' ' + to_string(nint(state->operand2, 2, state->operand2.length())) + " (x";
    }
    else if (state->alu_control_signal == 18) {
        state->memory_address = stoi(state->operand1, nullptr, 16) + nint(state->operand2, 2, state->operand2.length());
        state->is_mem = {0, 3};
        state->asm_code = "lb x" + std::to_string(state->rd) + ' ' + to_string(nint(state->operand2, 2, state->operand2.length())) + " (x";
    }
    else if (state->alu_control_signal == 19) { // Jalr
        state->register_data = nhex(state->PC + 4);
        state->return_address = nint(state->operand2, 2, state->operand2.length()) + nint(state->operand1, 16);
        pc_select = 1;
        state->pc_select = 1;
        state->asm_code = "jalr x" + to_string(state->rd) + " x" + std::to_string(state->rs1) + " " + std::to_string(nint(state->operand2, 2, state->operand2.length()));
    }
    else if (state->alu_control_signal == 20) {
        state->memory_address = stoi(state->operand1, nullptr, 16) + nint(state->operand2, 2, state->operand2.length());
        state->is_mem = {1, 0};
        state->asm_code = "sw x" + std::to_string(state->rs2) + ' ' + std::to_string(nint(state->operand2, 2, state->operand2.length())) + " (x";
    }
    else if (state->alu_control_signal == 22) {
        state->memory_address = stoi(state->operand1, nullptr, 16) + nint(state->operand2, 2, state->operand2.length());
        state->is_mem = {1, 1};
        state->asm_code = "sh x" + std::to_string(state->rs2) + ' ' + std::to_string(nint(state->operand2, 2, state->operand2.length())) + " (x" + std::to_string(state->rs1) + ")";
    }
    else if (state->alu_control_signal == 21) {
        state->memory_address = stoi(state->operand1, nullptr, 16) + nint(state->operand2, 2, state->operand2.length());;
        state->is_mem = {1, 3};
        state->asm_code = "sb x" + std::to_string(state->rs2) + ' ' + std::to_string(nint(state->operand2, 2, state->operand2.length())) + " (x" + std::to_string(state->rs1) + ")";
    }
    else if (state->alu_control_signal == 23) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        if (op1 == op2) {
            pc_offset = state->offset;
            state->inc_select = 1;
        }
        this->pc_offset = state->offset;
        this->inc_select = 1;
        state->asm_code = "beq x" + std::to_string(state->rs1) + " x" + std::to_string(state->rs2) + " " + std::to_string(this->pc_offset);
    }
    else if (state->alu_control_signal == 24) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        if (op1!=op2) {
            state->pc_offset = state->offset;
            state->inc_select = 1;
        }
        this->pc_offset = state->offset;
        this->inc_select = 1;
        state->asm_code = "bne x" + std::to_string(state->rs1) + " x" + std::to_string(state->rs2) + " " + std::to_string(this->pc_offset);
    }
    else if (state->alu_control_signal == 25) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        if (op1 >= op2) {
            state->pc_offset = state->offset;
            state->inc_select = 1;
        }
        this->pc_offset = state->offset;
        this->inc_select = 1;
        state->asm_code = "bge x" + std::to_string(state->rs1) + " x" + std::to_string(state->rs2) + " " + std::to_string(this->pc_offset);
    }
    else if (state->alu_control_signal == 26) {
        int op1 = nint(state->operand1, 16);
        int op2 = nint(state->operand2, 16);
        if (op1 < op2) {
            pc_offset = state->offset;
            inc_select = 1;
        }
        this->pc_offset = state->offset;
        this->inc_select = 1;
        state->asm_code = "blt x" + to_string(state->rs1) + " x" + std::to_string(state->rs2) + " " + to_string(this->pc_offset);
    }
    else if (state->alu_control_signal == 27) {
        state->register_data = nhex(state->PC + stoi(state->operand2, 0, 2));
        state->asm_code = "auipc x" + to_string(state->rd) + " " + std::to_string(std::stoi(state->operand2.substr(0, 20), nullptr, 2));
    }
    else if (state->alu_control_signal == 28) {
        state->register_data = nhex(stoi(state->operand2, nullptr, 2));
        state->asm_code = "lui x" + std::to_string(state->rd) + " " + to_string(stoi(state->operand2, 0, 2));
        
    }
    else if (state->alu_control_signal == 29) { // Jal
        state->register_data = nhex(state->PC + 4);
        this->pc_offset = state->offset;
        this->inc_select = 1;
        state->pc_offset = state->offset;
        state->inc_select = 1;
        state->asm_code = "jal x" + std::to_string(state->rd) + " " + std::to_string(this->pc_offset);
    }


    get_code[state->PC] = state->asm_code;

    if (state->register_data.length() > 10) {
        state->register_data = state->register_data.substr(0, 2) + state->register_data.substr(state->register_data.length() - 8);
    }

    state->register_data = state->register_data.substr(0, 2) +
        std::string(10 - state->register_data.length(), '0') + state->register_data.substr(2);
}

void Processor::mem(State *state) {
    if (!pipelining_enabled) {
        IAG(state);
    }

    if (state->is_dummy) return;

    if (state->is_mem[0] == -1) return;

    else if (state->is_mem[0] == 0) {
        if (MEM.find(state->memory_address) == MEM.end())
        {
            MEM[state->memory_address] = "00"; // default initialization
        }
        if (state->is_mem[1] == 0) {
            state->register_data = "0x" + MEM[state->memory_address];
        } else if (state->is_mem[1] == 1) {
            for (int i = 0; i < 2; i++)
            {
                if (MEM.find(state->memory_address + i) == MEM.end())
                    MEM[state->memory_address + i] = "00"; // ensure memory exists
            }
            state->register_data = "0x" + MEM[state->memory_address + 1] + MEM[state->memory_address];
        }
                // load word
    else if (state->is_mem[1] == 3)
    {
        for (int i = 0; i < 4; i++)
        {
            if (MEM.find(state->memory_address + i) == MEM.end())
                MEM[state->memory_address + i] = "00";
        }
        state->register_data = "0x" + MEM[state->memory_address + 3] + MEM[state->memory_address + 2] +
                        MEM[state->memory_address + 1] + MEM[state->memory_address];
    }
    // load double-word
    else if (state->is_mem[1] == 4)
    {
        for (int i = 0; i < 8; i++)
        {
            if (MEM.find(state->memory_address + i) == MEM.end())
                MEM[state->memory_address + i] = "00";
        }
        state->register_data = "0x" + MEM[state->memory_address + 7] + MEM[state->memory_address + 6] +
                        MEM[state->memory_address + 5] + MEM[state->memory_address + 4] +
                        MEM[state->memory_address + 3] + MEM[state->memory_address + 2] +
                        MEM[state->memory_address + 1] + MEM[state->memory_address];
    }

        string bin_data = hex_to_bin(state->register_data.substr(2)); //convert to binary. 
        int bit_length = (state->is_mem[1] == 0) ? 8 : (state->is_mem[1] == 1) ? 16 : (state->is_mem[1] == 3) ? 32 : 64;

        if (bit_length < 64)
        {
            bin_data = sign_extend(bin_data, bit_length);
        }

        state->register_data = bin_to_hex(bin_data); 

    } else {
        string store_data = (state->register_data.length() > 2) ? state->register_data.substr(2) : "00";
        if (state->is_mem[1] == 0)
        {
            if (store_data.length() < 2)
                store_data = "00" + store_data;
            MEM[state->memory_address] = store_data.substr(store_data.length() - 2, 2);
        }
        // store half-word
        else if (state->is_mem[1] == 1)
        {
            store_data = std::string(4 - store_data.length(), '0') + store_data;
            MEM[state->memory_address + 1] = store_data.substr(store_data.length() - 4, 2);
            MEM[state->memory_address] = store_data.substr(store_data.length() - 2, 2);
        }
        // store word
        else if (state->is_mem[1] == 3)
        {
            store_data = std::string(8 - store_data.length(), '0') + store_data;
            MEM[state->memory_address + 3] = store_data.substr(store_data.length() - 8, 2);
            MEM[state->memory_address + 2] = store_data.substr(store_data.length() - 6, 2);
            MEM[state->memory_address + 1] = store_data.substr(store_data.length() - 4, 2);
            MEM[state->memory_address] = store_data.substr(store_data.length() - 2, 2);
        }
        // store double-word
        else if (state->is_mem[1] == 4)
        {
            store_data = std::string(16 - store_data.length(), '0') + store_data;
            for (int i = 0; i < 8; i++)
            {
                MEM[state->memory_address + 7 - i] = store_data.substr(i * 2, 2);
            }
        }
    }
}

// Function to write back to the register file
void Processor::writeBack(State *state) {
    if (!state->is_dummy) {
        count_total_inst++;  // Total instructions

        // Check control instruction
        if (state->alu_control_signal == 19 || state->alu_control_signal == 23 ||
            state->alu_control_signal == 24 || state->alu_control_signal == 25 ||
            state->alu_control_signal == 26 || state->alu_control_signal == 29) {
            count_control_inst++;
        }
        // Check data transfer instruction
        else if (state->alu_control_signal == 16 || state->alu_control_signal == 17 ||
                state->alu_control_signal == 18 || state->alu_control_signal == 20 ||
                state->alu_control_signal == 21 || state->alu_control_signal == 22 || state->alu_control_signal == 30 || state->alu_control_signal == 31) {
            count_mem_inst++;
        }
        // Default: ALU instruction
        else {
            count_alu_inst++;
        }

        // Write back if signal is set
        if (state->write_back_signal) {
            if (state->rd != 0) {
                R[state->rd] = state->register_data;  // Write the data to the register file
            }
        }
    }
}

// Exit the simulation and write results to files
void Processor::swi_exit()
{
    write_data_memory();
    terminate = true;
}