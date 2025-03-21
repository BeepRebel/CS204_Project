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

// Memory write
void write_word(const std::string& address, const std::string& instruction) {
    int idx = std::stoi(address.substr(2), nullptr, 16);
    MEM[idx] = instruction.substr(8, 2);
    MEM[idx + 1] = instruction.substr(6, 2);
    MEM[idx + 2] = instruction.substr(4, 2);
    MEM[idx + 3] = instruction.substr(2, 2);
}