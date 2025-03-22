#include <bits/stdc++.h>
#include "../include/myARMSim.h"
using namespace std;

// Structure: {opcode, {funcKey, {operation, alu_control_signal, instruction_type}}}
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

// Register file - 32 registers (x0 to x31)
string X[32];

int clock_cycles = 0; //cycle counter

unsigned int PC = 0; //program counter

unordered_map<unsigned int, string> MEM; // memory stored in the form of unordered map.

// data path and control path signal. 
unsigned int memory_address = 0;
int alu_control_signal = -1;
vector<int> is_mem{-1, -1}; // this stores the type of memory instruction. 
bool write_back_signal = false; //write back signal for the mux.
bool terminate1 = false;
int inc_select = 0; //mux select line
int pc_select = 0;  // mux select line
int return_address = -1;
int pc_offset = 0;

string instruction_word = "0x00000000";
string operand1 = "0x00000000";
string operand2 = "0x00000000";
string operation = "";
string rd = "0";
string offset = "0";
string register_data = "0x00000000";

// utility: to convert int to hex format
string nhex(int num)
{
    // this is to handle negative numbers
    if (num < 0)
    {
        num += (1LL << 32);
    }

    stringstream ss;
    ss << "0x" << setfill('0') << setw(8) << hex << num << dec;
    return ss.str();
}

// utility: to convert to int with sign extension.
int nint(const string &s, int base, int bits = 32)
{
    // convert string to long long unsigned.
    unsigned long long num = stoull(s, nullptr, base);

    // this is to sign extend
    if (num >= (1ULL << (bits - 1)))
    {
        num -= (1ULL << bits);
    }

    return static_cast<int>(num);
}

string sign_extend(std::string data, int bit_length)
{
    if (data.substr(0, 2) == "0x") // check if the input is in hexadecimal format
    {                                                             // handling hexadecimal input
        char highDigit = data[2];                                 // first digit after "0x"
        bool isNegative = (highDigit >= '8' && highDigit <= 'f'); // checking if the number is negative based on the most significant digit

        if (isNegative)
        {
            // if the number is negative, extend with 'f' to preserve sign
            data = "0x" + std::string(8 - (data.length() - 2), 'f') + data.substr(2); 
        }
        else
        {
            // if the number is positive, extend with '0' to preserve sign
            data = "0x" + std::string(8 - (data.length() - 2), '0') + data.substr(2); 
        }
    }
    else
    { // handling binary input
        if (data.length() < bit_length)
        {
            char sign_bit = data[0]; // get the sign bit ('1' for negative, '0' for positive)
            // extend the sign bit to match the required bit length
            data = std::string(bit_length - data.length(), sign_bit) + data;
        }
    }
    return data; // return the sign-extended value
}

// Utility function: Convert binary string to hex string
string bin_to_hex(const string &binary)
{
    stringstream ss;
    ss << "0x" << hex << stoull(binary, nullptr, 2) << dec;
    return ss.str();
}

string hex_to_bin(const string &hex)
{
    string bin = ""; // initialize an empty string to store the binary representation
    for (char c : hex)
    {
        // convert the hexadecimal character to its integer value
        int value = (c >= '0' && c <= '9') ? (c - '0') : (10 + (c - 'a'));
        
        // convert the integer value to a 4-bit binary string and append to the result
        bin += std::bitset<4>(value).to_string(); 
    }
    return bin; // return the binary representation of the hexadecimal input
}

// Reset processor state - initialize registers
void reset_proc()
{
    // initialize all registers to zero
    for (int i = 0; i < 32; i++)
    {
        X[i] = "0x00000000";
    }

    // set x2 and x3 to initial values.
    X[2] = "0x7FFFFFDC";
    X[3] = "0x10000000";
}

// main simulation function that executes the RISC-V program
void run_RISCVsim()
{
    while (true)
    {
        fetch();
        decode();

        if (terminate1)
        {
            return;
        }

        execute();

        if (terminate1)
        {
            return;
        }

        mem();
        write_back();

        clock_cycles++;

        cout << "CLOCK CYCLE: " << clock_cycles << endl
             << endl;
    }
}

// load program from memory file
void load_program_memory(const string &file_name)
{
    ifstream infile(file_name);

    // check if the file is open, otherwise print an error and exit
    if (!infile)
    {
        cerr << "ERROR: error opening input file" << endl;
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

// Write memory contents to output files
void write_data_memory()
{
    // write data memory to data_out.mc
    ofstream data_out("data_out.mc");
    if (!data_out.is_open())
    {
        cout << "ERROR: Error opening data_out.mc file for writing" << endl;
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

    // write register contents to reg_out.mc
    ofstream reg_out("reg_out.mc");
    if (!reg_out.is_open())
    {
        cout << "ERROR: Error opening reg_out.mc file for writing" << endl;
        return;
    }

    // iterate through all 32 registers and write their values
    for (int i = 0; i < 32; i++)
    {
        reg_out << "x" << dec << i << " " << X[i] << endl;
    }

    reg_out.close();
}

// Fetch stage: Read instruction from memory
void fetch()
{
    // construct 32-bit instruction from 4 bytes in memory (little-endian)
    string byte0 = MEM.count(PC) ? MEM[PC] : "00";
    string byte1 = MEM.count(PC + 1) ? MEM[PC + 1] : "00";
    string byte2 = MEM.count(PC + 2) ? MEM[PC + 2] : "00";
    string byte3 = MEM.count(PC + 3) ? MEM[PC + 3] : "00";

    // combine bytes to form the instruction word
    instruction_word = "0x" + byte3 + byte2 + byte1 + byte0;

    // check if instruction is a halt instruction (all zeros)
    if (instruction_word == "0x00000000")
    {
        swi_exit(); // terminate execution
        return;
    }

    // print fetched instruction and its address
    cout << "FETCH: Fetch instruction " << instruction_word << " from address " << nhex(PC) << endl;

    // reset pc increment and selection signals
    inc_select = 0;
    pc_select = 0;
}

// Decode stage: Identify instruction type and extract operands
void decode()
{
    // instruction to end the simulation
    if (instruction_word == "0x00000000")
    {
        cout << "END SIMULATION" << endl
             << endl;
        swi_exit();
        return;
    }
    
    string hex_part = instruction_word.substr(2); // Remove '0x' prefix
    string bin_instruction;

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
    alu_control_signal = -1;
    is_mem = {-1, -1};
    
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
            operation = get<0>(func_it->second);
            alu_control_signal = get<1>(func_it->second);
            op_type = get<2>(func_it->second);
            found = true;
        }
        
        if (!found) {
            cout << "ERROR: Unidentifiable machine code (valid opcode but invalid func3/func7)!" << endl;
            swi_exit();
            return;
        }
    } else {
        cout << "ERROR: Unidentifiable machine code (invalid opcode)!" << endl;
        swi_exit();
        return;
    }

    // Extract operands based on instruction type
    if (op_type == "R")
    {
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
    else if (op_type == "I")
    {
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
    else if (op_type == "S")
    {
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
    else if (op_type == "SB")
    {
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
    else if (op_type == "U")
    {
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
    else if (op_type == "UJ")
    {
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
    else
    {
        cout << "ERROR: Unidentifiable machine code!" << endl;
        swi_exit();
        return;
    }
}

// Helper function to perform binary operations
string performBinaryOp(const std::string& op1, const std::string& op2, 
    function<int(int, int)> operation) {
    int result = operation(nint(op1, 16), nint(op2, 16));
    return nhex(result);
}

// Helper function for handling shifts
string performShift(const std::string& op1, const std::string& op2, 
    function<string(int, int)> shiftOperation) {
    if (nint(op2, 16) < 0) {
    std::cout << "ERROR: Shift by negative!\n" << std::endl;
    swi_exit();
    return "";
    }
    return shiftOperation(std::stoi(op1, nullptr, 16), std::stoi(op2, nullptr, 16));
}

// Helper function for arithmetic right shift
string arithmeticRightShift(int value, int shift) {
    int result = value >> shift;
    string bin_value = std::bitset<32>(result).to_string();

    // Check if the original value was negative (check the MSB)
    if ((value & 0x80000000) != 0) {
    // Fill with 1s for sign extension
        for (int i = 0; i < shift; i++) {
        bin_value = '1' + bin_value;
        }
    }
    return nhex(std::stoi(bin_value, nullptr, 2));
}

// Helper function for setting memory access mode
void setMemoryAccess(int address, int accessType, int width) {
    memory_address = address;
    is_mem[0] = accessType;
    is_mem[1] = width;
}

// Helper function for branch operations
bool evaluateBranchCondition(const std::string& op1, const std::string& op2, 
     function<bool(int, int)> condition) {
     return condition(nint(op1, 16), nint(op2, 16));
}

// Helper function for address calculation
int calculateAddress(const std::string& base, const std::string& offset) {
    return stoi(base, nullptr, 16) + nint(offset, 2, offset.length());
}

// Helper function to handle immediate operations 
string performImmediateOp(const std::string& op1, const std::string& op2, 
       function<int(int, int)> operation) {
       return nhex(operation(std::stoi(op1, nullptr, 16), nint(op2, 2, op2.length())));
}

// Helper function to format output string
string formatOutput(const std::string& result) {
        string formatted = result;
        if (formatted.length() > 10) {
        formatted = formatted.substr(0, 2) + formatted.substr(formatted.length() - 8);
    }

    // Zero-pad the result
    formatted = formatted.substr(0, 2) +
    std::string(10 - formatted.length(), '0') +
    formatted.substr(2);

    return formatted;
}

// Log shift operations
void logShiftOperation(const std::string& value, int shift, bool withAdd = false) {
    std::string message = "EXECUTE: Shift left " + std::to_string(std::stoi(value.substr(0, 20), 0, 2)) 
    + " by 12 bits";
    if (withAdd) {
        message += " and ADD " + to_string(PC + 4);
    }
    cout << message << endl;
}

// Main execute function
void execute() {
    switch (alu_control_signal) {
        // AND operation
        case 1: {
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a & b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // ADD operation
        case 2: {
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a + b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // OR operation
        case 3: {
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a | b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // SHIFT_LEFT operation
        case 4: {
        register_data = performShift(operand1, operand2, [](int a, int b) { 
        return nhex(a << b); 
        });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // SET_LESS_THAN operation
        case 5: {
        register_data = (nint(operand1, 16) < nint(operand2, 16)) ? "0x1" : "0x0";
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // SHIFT_RIGHT_ARITHMETIC operation
        case 6: {
        register_data = performShift(operand1, operand2, [](int a, int b) {
        return arithmeticRightShift(a, b);
        });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // SHIFT_RIGHT_LOGICAL operation
        case 7: {
        register_data = performShift(operand1, operand2, [](int a, int b) { 
        return nhex(a >> b); 
        });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // SUB operation
        case 8: {
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a - b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // XOR operation
        case 9: {
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a ^ b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // MUL operation
        case 10: {
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a * b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // DIV operation
        case 11: {
        if (nint(operand2, 16) == 0) {
        std::cout << "ERROR: Division by zero!\n" << std::endl;
        swi_exit();
        return;
        }
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a / b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // MOD operation
        case 12: {
        register_data = performBinaryOp(operand1, operand2, [](int a, int b) { return a % b; });
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // AND_IMM operation
        case 13: {
        register_data = nhex(std::stoi(operand1, nullptr, 16) & std::stoi(operand2, 0, 2));
        std::cout << "EXECUTE: AND " << std::stoi(operand1, nullptr, 16) 
        << " and " << nint(operand2, 2, operand2.length()) << std::endl;
        break;
        }

        // ADD_IMM operation
        case 14: {
        register_data = performImmediateOp(operand1, operand2, [](int a, int b) { return a + b; });
        std::cout << "EXECUTE: ADD " << std::stoi(operand1, nullptr, 16) 
        << " and " << nint(operand2, 2, operand2.length()) << std::endl;
        break;
        }

        // OR_IMM operation
        case 15: {
        register_data = nhex(std::stoi(operand1, nullptr, 16) | std::stoi(operand2, 0, 2));
        std::cout << "EXECUTE: OR " << std::stoi(operand1, nullptr, 16) 
        << " and " << nint(operand2, 2, operand2.length()) << std::endl;
        break;
        }

        // LOAD_WORD operation
        case 16: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 0, 0); // load (0), word (0)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }

        // LOAD_HALF operation
        case 17: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 0, 1); // load (0), half (1)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }

        // LOAD_BYTE operation
        case 18: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 0, 3); // load (0), byte (3)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }

        // JUMP_AND_LINK operation
        case 19: {
        register_data = nhex(PC + 4);
        return_address = nint(operand2, 2, operand2.length()) + nint(operand1, 16);
        pc_select = 1;
        cout << "EXECUTE: No execute operation" << endl;
        break;
        }

        // STORE_WORD operation
        case 20: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 1, 0); // store (1), word (0)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }

        // STORE_BYTE operation
        case 21: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 1, 3); // store (1), byte (3)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }

        // STORE_HALF operation
        case 22: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 1, 1); // store (1), half (1)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }

        // BRANCH_EQUAL operation
        case 23: {
        if (evaluateBranchCondition(operand1, operand2, [](int a, int b) { return a == b; })) {
        pc_offset = nint(offset, 2, offset.length());
        inc_select = 1;
        }
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // BRANCH_NOT_EQUAL operation
        case 24: {
        if (evaluateBranchCondition(operand1, operand2, [](int a, int b) { return a != b; })) {
        pc_offset = nint(offset, 2, offset.length());
        inc_select = 1;
        }
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // BRANCH_GE operation
        case 25: {
        if (evaluateBranchCondition(operand1, operand2, [](int a, int b) { return a >= b; })) {
        pc_offset = nint(offset, 2, offset.length());
        inc_select = 1;
        }
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // BRANCH_LT operation
        case 26: {
        if (evaluateBranchCondition(operand1, operand2, [](int a, int b) { return a < b; })) {
        pc_offset = nint(offset, 2, offset.length());
        inc_select = 1;
        }
        cout << "EXECUTE: " << operation << " " << nint(operand1, 16) << " and " << nint(operand2, 16) << endl;
        break;
        }

        // AUIPC operation
        case 27: {
        register_data = nhex(PC + 4 + std::stoi(operand2, 0, 2));
        std::cout << "EXECUTE: Shift left " << std::stoi(operand2.substr(0, 20), 0, 2) 
        << " by 12 bits and ADD " << (PC + 4) << std::endl;
        break;
        }

        // LUI operation
        case 28: {
        register_data = nhex(std::stoi(operand2, 0, 2));
        logShiftOperation(operand2, 12);
        break;
        }

        // JAL operation
        case 29: {
        register_data = nhex(PC + 4);
        pc_offset = nint(offset, 2, offset.length());
        inc_select = 1;
        cout << "EXECUTE: No execute operation" << endl;
        break;
        }

        // LOAD_UNSIGNED_BYTE operation
        case 30: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 0, 4); // load (0), unsigned byte (4)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }

        // STORE_UNSIGNED_BYTE operation
        case 31: {
        int address = calculateAddress(operand1, operand2);
        setMemoryAccess(address, 1, 4); // store (1), unsigned byte (4)
        cout << "EXECUTE: " << "ADD" << " " << stoi(operand1, nullptr, 16) << " and " << nint(operand2, 2, operand2.length()) << endl;
        break;
        }
    }

    if (register_data.length() > 10) {
    register_data = register_data.substr(0, 2) + register_data.substr(register_data.length() - 8);
    } //to format the register data to correct size.

    register_data = register_data.substr(0, 2) +
    string(10 - register_data.length(), '0') +
    register_data.substr(2); //this is to zero-pad the register data
}


// Performs the memory operations and also performs the operations of IAG.
void mem()
{
    if (is_mem[0] == -1) // check if there is no memory operation
    {
        cout << "MEMORY: No memory operation" << std::endl;
    }
    else if (is_mem[0] == 0) // handle load operation
    {
        // ensure memory is initialized before reading
        if (MEM.find(memory_address) == MEM.end())
        {
            MEM[memory_address] = "00"; // default initialization
        }

        // load byte
        if (is_mem[1] == 0)
        {
            register_data = "0x" + MEM[memory_address];
        }
        // load half-word
        else if (is_mem[1] == 1)
        {
            for (int i = 0; i < 2; i++)
            {
                if (MEM.find(memory_address + i) == MEM.end())
                    MEM[memory_address + i] = "00"; // ensure memory exists
            }
            register_data = "0x" + MEM[memory_address + 1] + MEM[memory_address];
        }
        // load word
        else if (is_mem[1] == 3)
        {
            for (int i = 0; i < 4; i++)
            {
                if (MEM.find(memory_address + i) == MEM.end())
                    MEM[memory_address + i] = "00";
            }
            register_data = "0x" + MEM[memory_address + 3] + MEM[memory_address + 2] +
                            MEM[memory_address + 1] + MEM[memory_address];
        }
        // load double-word
        else if (is_mem[1] == 4)
        {
            for (int i = 0; i < 8; i++)
            {
                if (MEM.find(memory_address + i) == MEM.end())
                    MEM[memory_address + i] = "00";
            }
            register_data = "0x" + MEM[memory_address + 7] + MEM[memory_address + 6] +
                            MEM[memory_address + 5] + MEM[memory_address + 4] +
                            MEM[memory_address + 3] + MEM[memory_address + 2] +
                            MEM[memory_address + 1] + MEM[memory_address];
        }

        string bin_data = hex_to_bin(register_data.substr(2)); //convert to binary. 
        int bit_length = (is_mem[1] == 0) ? 8 : (is_mem[1] == 1) ? 16 : (is_mem[1] == 3) ? 32 : 64;

        if (bit_length < 64)
        {
            bin_data = sign_extend(bin_data, bit_length);
        }

        register_data = bin_to_hex(bin_data);

        cout << "MEMORY: Load(" << ((is_mem[1] == 0) ? "byte" : (is_mem[1] == 1) ? "half-word" : (is_mem[1] == 3) ? "word" : "doubleword")
                  << ") " << nint(register_data, 16) << " from " << std::hex << memory_address << dec << endl;
    }
    else // handle store operation
    {
        // ensure proper data formatting
        string store_data = (register_data.length() > 2) ? register_data.substr(2) : "00";

        // store byte
        if (is_mem[1] == 0)
        {
            if (store_data.length() < 2)
                store_data = "00" + store_data;
            MEM[memory_address] = store_data.substr(store_data.length() - 2, 2);
        }
        // store half-word
        else if (is_mem[1] == 1)
        {
            store_data = std::string(4 - store_data.length(), '0') + store_data;
            MEM[memory_address + 1] = store_data.substr(store_data.length() - 4, 2);
            MEM[memory_address] = store_data.substr(store_data.length() - 2, 2);
        }
        // store word
        else if (is_mem[1] == 3)
        {
            store_data = std::string(8 - store_data.length(), '0') + store_data;
            MEM[memory_address + 3] = store_data.substr(store_data.length() - 8, 2);
            MEM[memory_address + 2] = store_data.substr(store_data.length() - 6, 2);
            MEM[memory_address + 1] = store_data.substr(store_data.length() - 4, 2);
            MEM[memory_address] = store_data.substr(store_data.length() - 2, 2);
        }
        // store double-word
        else if (is_mem[1] == 4)
        {
            store_data = std::string(16 - store_data.length(), '0') + store_data;
            for (int i = 0; i < 8; i++)
            {
                MEM[memory_address + 7 - i] = store_data.substr(i * 2, 2);
            }
        }

        cout << "MEMORY: Store(" << ((is_mem[1] == 0) ? "byte" : (is_mem[1] == 1) ? "half-word" : (is_mem[1] == 3) ? "word" : "doubleword")
                  << ") " << nint(register_data, 16) << " to " << std::hex << memory_address << dec << std::endl;
    }

    // update pc according to control signals
    if (pc_select)
    {
        PC = return_address;
    }
    else if (inc_select)
    {
        PC += pc_offset;
    }
    else
    {
        PC += 4;
    }
}

// Writes the results back to the register file
void write_back()
{
    if (write_back_signal)
    {
        if (stoi(rd, nullptr, 2) != 0)
        {
            X[stoi(rd, nullptr, 2)] = register_data;
            cout << "WRITEBACK: Write " << nint(register_data, 16) << " to X" << std::stoi(rd, nullptr, 2) << endl;
        }
        else
        {
            cout << "WRITEBACK: Value of X0 can not change" << endl;
        }
    }
    else
    {
        cout << "WRITEBACK: No write-back operation" << endl;
    }
}

// Memory write
void write_word(const std::string &address, const std::string &instruction)
{
    int idx = std::stoi(address.substr(2), nullptr, 16);
    MEM[idx] = instruction.substr(8, 2);
    MEM[idx + 1] = instruction.substr(6, 2);
    MEM[idx + 2] = instruction.substr(4, 2);
    MEM[idx + 3] = instruction.substr(2, 2);
}

// Exit the simulation and write results to files
void swi_exit()
{
    write_data_memory();
    terminate1 = true;
}
