#include <bits/stdc++.h>
#include "../include/myARMSim.h"
using namespace std;

// Register file - 32 registers (x0 to x31)
string X[32];

// Clock cycles counter
int clock_cycles = 0;

// Program Counter
unsigned int PC = 0;

// Memory implementation using unordered_map (similar to Python's defaultdict)
unordered_map<unsigned int, string> MEM;

// Intermediate datapath and control path signals
string instruction_word = "0x00000000";
string operand1 = "0x00000000";
string operand2 = "0x00000000";
string operation = "";
string rd = "0";
string offset = "0";
string register_data = "0x00000000";
unsigned int memory_address = 0;
int alu_control_signal = -1;
vector<int> is_mem{-1, -1};
bool write_back_signal = false;
bool terminate1 = false;
int inc_select = 0;
int pc_select = 0;
int return_address = -1;
int pc_offset = 0;

// Utility function: Convert number to hex string with 0x prefix
string nhex(int num) {
    // Handle negative numbers by adding 2^32
    if (num < 0) {
        num += (1LL << 32);
    }
    
    stringstream ss;
    ss << "0x" << setfill('0') << setw(8) << hex << num;
    return ss.str();
}

// Utility function: Convert hex/binary string to integer with sign extension
int nint(const string& s, int base, int bits = 32) {
    // Convert string to unsigned long long
    unsigned long long num = stoull(s, nullptr, base);
    
    // Sign extension if the highest bit is set
    if (num >= (1ULL << (bits - 1))) {
        num -= (1ULL << bits);
    }
    
    return static_cast<int>(num);
}

// Utility function: Convert binary string to hex string
string bin_to_hex(const string& binary) {
    stringstream ss;
    ss << "0x" << hex << stoull(binary, nullptr, 2);
    return ss.str();
}

// Main simulation function that executes the RISC-V program
void run_RISCVsim() {
    while (true) {
        fetch();
        decode();
        
        if (terminate1) {
            return;
        }
        
        execute();
        
        if (terminate1) {
            return;
        }
        
        mem();
        write_back();
        
        clock_cycles++;
        
        cout << "CLOCK CYCLE: " << clock_cycles << endl << endl;
    }
}

// Reset processor state - initialize registers
void reset_proc() {
    // Initialize all registers to zero
    for (int i = 0; i < 32; i++) {
        X[i] = "0x00000000";
    }
    
    // Set stack pointer (x2) and global pointer (x3) to initial values
    X[2] = "0x7FFFFFDC";
    X[3] = "0x10000000";
}

// Load program from memory file
void load_program_memory(const string& file_name) {
    ifstream infile(file_name);
    
    if (!infile.is_open()) {
        cout << "ERROR: Error opening input .mc file" << endl;
        exit(1);
    }
    
    string line;
    int line_number = 0;
    while (getline(infile, line)) {
        istringstream iss(line);
        string address, instruction;
        
        if (iss >> address >> instruction) {
            write_word(address, instruction);
            cout << "written line:" << line_number;

        }
    }
    
    infile.close();
}

// Exit the simulation and write results to files
void swi_exit() {
    write_data_memory();
    terminate1 = true;
}

// Fetch stage: Read instruction from memory
void fetch() {
    // Construct 32-bit instruction from 4 bytes in memory (little-endian)
    string byte0 = MEM.count(PC) ? MEM[PC] : "00";
    string byte1 = MEM.count(PC+1) ? MEM[PC+1] : "00";
    string byte2 = MEM.count(PC+2) ? MEM[PC+2] : "00";
    string byte3 = MEM.count(PC+3) ? MEM[PC+3] : "00";
    
    instruction_word = "0x" + byte3 + byte2 + byte1 + byte0;
    
    cout << "FETCH: Fetch instruction " << instruction_word << " from address " << nhex(PC) << endl;
    
    // Reset PC increment and selection signals
    inc_select = 0;
    pc_select = 0;
}

// Decode stage: Identify instruction type and extract operands
void decode() {
    // Special instruction for program termination
    if (instruction_word == "0x00000000") {
        cout << "END PROGRAM" << endl << endl;
        swi_exit();
        return;
    }
    
    // Convert hex instruction to binary string
    string hex_part = instruction_word.substr(2); // Remove '0x' prefix
    stringstream ss;
    for (int i = 0; i < hex_part.length(); i++) {
        char c = hex_part[i];
        int value;
        if (c >= '0' && c <= '9') value = c - '0';
        else value = 10 + (c - 'a');
        
        ss << setw(4) << setfill('0') << bitset<4>(value).to_string();
    }
    string bin_instruction = ss.str();
    
    // Ensure bin_instruction is 32 bits
    while (bin_instruction.length() < 32) {
        bin_instruction = "0" + bin_instruction;
    }
    
    // Extract opcode, func3, and func7 fields
    int opcode = stoi(bin_instruction.substr(25, 7), nullptr, 2);
    int func3 = stoi(bin_instruction.substr(17, 3), nullptr, 2);
    int func7 = stoi(bin_instruction.substr(0, 7), nullptr, 2);
    
    // In a real implementation, we would read instruction mapping from a CSV file
    // For this conversion, we'll use a simplified decoder function
    
    // Simplified instruction decoder
    string op_type;
    alu_control_signal = -1;
    is_mem = {-1, -1};
    
    // Identify instruction type based on opcode, func3, and func7
    if (opcode == 0b0110011) { // R-type
        op_type = "R";
        if (func3 == 0 && func7 == 0) {
            operation = "add";
            alu_control_signal = 2;
        } else if (func3 == 0 && func7 == 0b0100000) {
            operation = "sub";
            alu_control_signal = 8;
        } else if (func3 == 0b111 && func7 == 0) {
            operation = "and";
            alu_control_signal = 1;
        } else if (func3 == 0b110 && func7 == 0) {
            operation = "or";
            alu_control_signal = 3;
        } else if (func3 == 0b001 && func7 == 0) {
            operation = "sll";
            alu_control_signal = 4;
        } else if (func3 == 0b010 && func7 == 0) {
            operation = "slt";
            alu_control_signal = 5;
        } else if (func3 == 0b101 && func7 == 0b0100000) {
            operation = "sra";
            alu_control_signal = 6;
        } else if (func3 == 0b101 && func7 == 0) {
            operation = "srl";
            alu_control_signal = 7;
        } else if (func3 == 0b100 && func7 == 0) {
            operation = "xor";
            alu_control_signal = 9;
        } else if (func3 == 0b000 && func7 == 0b0000001) {
            operation = "mul";
            alu_control_signal = 10;
        } else if (func3 == 0b100 && func7 == 0b0000001) {
            operation = "div";
            alu_control_signal = 11;
        } else if (func3 == 0b110 && func7 == 0b0000001) {
            operation = "rem";
            alu_control_signal = 12;
        }
    } else if (opcode == 0b0010011) { // I-type ALU
        op_type = "I";
        if (func3 == 0) {
            operation = "addi";
            alu_control_signal = 14;
        } else if (func3 == 0b111) {
            operation = "andi";
            alu_control_signal = 13;
        } else if (func3 == 0b110) {
            operation = "ori";
            alu_control_signal = 15;
        }
    } else if (opcode == 0b0000011) { // I-type Load
        op_type = "I";
        if (func3 == 0) {
            operation = "lb";
            alu_control_signal = 16;
        } else if (func3 == 0b001) {
            operation = "lh";
            alu_control_signal = 17;
        } else if (func3 == 0b010) {
            operation = "lw";
            alu_control_signal = 18;
        }else if (func3 == 0b011){
            operation = "ld";
             alu_control_signal = 30;
        }
    } else if (opcode == 0b1100111 && func3 == 0) { // JALR
        op_type = "I";
        operation = "jalr";
        alu_control_signal = 19;
    } else if (opcode == 0b0100011) { // S-type
        op_type = "S";
        if (func3 == 0) {
            operation = "sb";
            alu_control_signal = 20;
        } else if (func3 == 0b001) {
            operation = "sh";
            alu_control_signal = 22;
        } else if (func3 == 0b010) {
            operation = "sw";
            alu_control_signal = 21;
        } else if (func3 == 0b011){
            operation = "sd";
            alu_control_signal = 31;
        }
    } else if (opcode == 0b1100011) { // SB-type
        op_type = "SB";
        if (func3 == 0) {
            operation = "beq";
            alu_control_signal = 23;
        } else if (func3 == 0b001) {
            operation = "bne";
            alu_control_signal = 24;
        } else if (func3 == 0b101) {
            operation = "bge";
            alu_control_signal = 25;
        } else if (func3 == 0b100) {
            operation = "blt";
            alu_control_signal = 26;
        }
    } else if (opcode == 0b0010111) { // AUIPC
        op_type = "U";
        operation = "auipc";
        alu_control_signal = 27;
    } else if (opcode == 0b0110111) { // LUI
        op_type = "U";
        operation = "lui";
        alu_control_signal = 28;
    } else if (opcode == 0b1101111) { // JAL
        op_type = "UJ";
        operation = "jal";
        alu_control_signal = 29;
    }
    
    if (alu_control_signal == -1) {
        cout << "ERROR: Unidentifiable machine code!" << endl;
        swi_exit();
        return;
    }
    
    // Extract operands based on instruction type
    if (op_type == "R") {
        // Extract register fields
        string rs2 = bin_instruction.substr(7, 5);
        string rs1 = bin_instruction.substr(12, 5);
        rd = bin_instruction.substr(20, 5);
        
        // Read values from registers
        operand1 = X[stoi(rs1, nullptr, 2)];
        operand2 = X[stoi(rs2, nullptr, 2)];
        write_back_signal = true;
        
        cout << "DECODE: Operation is " << operation << ", first operand is X" 
             << stoi(rs1, nullptr, 2) << ", second operand is X" << stoi(rs2, nullptr, 2) 
             << ", destination register is X" << stoi(rd, nullptr, 2) << endl;
        
        cout << "DECODE: Read registers: X" << stoi(rs1, nullptr, 2) << " = " 
             << nint(operand1, 16) << ", X" << stoi(rs2, nullptr, 2) << " = " 
             << nint(operand2, 16) << endl;
    } 
    else if (op_type == "I") {
        // Extract register fields and immediate
        string rs1 = bin_instruction.substr(12, 5);
        rd = bin_instruction.substr(20, 5);
        string imm = bin_instruction.substr(0, 12);
        
        operand1 = X[stoi(rs1, nullptr, 2)];
        operand2 = imm;
        write_back_signal = true;
        
        cout << "DECODE: Operation is " << operation << ", first operand is X" 
             << stoi(rs1, nullptr, 2) << ", immediate is " << nint(operand2, 2, operand2.length()) 
             << ", destination register is X" << stoi(rd, nullptr, 2) << endl;
        
        cout << "DECODE: Read registers: X" << stoi(rs1, nullptr, 2) << " = " 
             << nint(operand1, 16) << endl;
    } 
    else if (op_type == "S") {
        // Extract register fields and immediate
        string rs2 = bin_instruction.substr(7, 5);
        string rs1 = bin_instruction.substr(12, 5);
        string imm = bin_instruction.substr(0, 7) + bin_instruction.substr(20, 5);
        
        operand1 = X[stoi(rs1, nullptr, 2)];
        operand2 = imm;
        register_data = X[stoi(rs2, nullptr, 2)];
        write_back_signal = false;
        
        cout << "DECODE: Operation is " << operation << ", first operand is X" 
             << stoi(rs1, nullptr, 2) << ", immediate is " << nint(operand2, 2, operand2.length())
             << ", data to be stored is in X" << stoi(rs2, nullptr, 2) << endl;
        
        cout << "DECODE: Read registers: X" << stoi(rs1, nullptr, 2) << " = " 
             << nint(operand1, 16) << ", X" << stoi(rs2, nullptr, 2) << " = " 
             << nint(register_data, 16) << endl;
    } 
    else if (op_type == "SB") {
        // Extract register fields and immediate for branch instructions
        string rs2 = bin_instruction.substr(7, 5);
        string rs1 = bin_instruction.substr(12, 5);
        
        operand1 = X[stoi(rs1, nullptr, 2)];
        operand2 = X[stoi(rs2, nullptr, 2)];
        
        // Branch immediate format is complex in RISC-V
        string imm = bin_instruction.substr(0, 1) + bin_instruction.substr(24, 1) + 
                    bin_instruction.substr(1, 6) + bin_instruction.substr(20, 4) + "0";
        
        offset = imm;
        write_back_signal = false;
        
        cout << "DECODE: Operation is " << operation << ", first operand is X" 
             << stoi(rs1, nullptr, 2) << ", second operand is X" << stoi(rs2, nullptr, 2) 
             << ", immediate is " << nint(offset, 2, offset.length()) << endl;
        
        cout << "DECODE: Read registers: X" << stoi(rs1, nullptr, 2) << " = " 
             << nint(operand1, 16) << ", X" << stoi(rs2, nullptr, 2) << " = " 
             << nint(operand2, 16) << endl;
    } 
    else if (op_type == "U") {
        // U-type instructions (LUI, AUIPC) with upper immediate
        rd = bin_instruction.substr(20, 5);
        string imm = bin_instruction.substr(0, 20);
        
        write_back_signal = true;
        
        cout << "DECODE: Operation is " << operation << ", immediate is " 
             << nint(imm, 2, imm.length()) << ", destination register is X" 
             << stoi(rd, nullptr, 2) << endl;
        
        cout << "DECODE: No register read" << endl;
        
        // Add 12 zeros to the right (shift left by 12)
        imm += string(12, '0');
        operand2 = imm;
    } 
    else if (op_type == "UJ") {
        // UJ-type instructions (JAL)
        rd = bin_instruction.substr(20, 5);
        
        // JAL immediate format is complex in RISC-V
        string imm = bin_instruction.substr(0, 1) + bin_instruction.substr(12, 8) + 
                    bin_instruction.substr(11, 1) + bin_instruction.substr(1, 10);
        
        write_back_signal = true;
        
        cout << "DECODE: Operation is " << operation << ", immediate is " 
             << nint(imm, 2, imm.length()) << ", destination register is X" 
             << stoi(rd, nullptr, 2) << endl;
        
        cout << "DECODE: No register read" << endl;
        
        // Append a 0 to the right (shift left by 1)
        imm += "0";
        offset = imm;
    } 
    else {
        cout << "ERROR: Unidentifiable machine code!" << endl;
        swi_exit();
        return;
    }
}

// Memory write
void write_word(const std::string& address, const std::string& instruction) {
    int idx = std::stoi(address.substr(2), nullptr, 16);
    MEM[idx] = instruction.substr(8, 2);
    MEM[idx + 1] = instruction.substr(6, 2);
    MEM[idx + 2] = instruction.substr(4, 2);
    MEM[idx + 3] = instruction.substr(2, 2);
}