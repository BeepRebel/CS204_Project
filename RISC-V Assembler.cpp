// RISC-V Assembler
#include <bits/stdc++.h>
using namespace std;

// Instruction formats
enum class Format { R, I, S, SB, U, UJ };

// Instruction information
struct Instruction {
    string name;
    Format format;
    uint32_t opcode;
    uint32_t funct3;
    uint32_t funct7;
};

// Instruction set
std::map<std::string, Instruction> instructions = {
    {"add",   {"add",   Format::R,  0b0110011, 0b000, 0b0000000}},
    {"and",   {"and",   Format::R,  0b0110011, 0b111, 0b0000000}},
    {"or",    {"or",    Format::R,  0b0110011, 0b110, 0b0000000}},
    {"sll",   {"sll",   Format::R,  0b0110011, 0b001, 0b0000000}},
    {"slt",   {"slt",   Format::R,  0b0110011, 0b010, 0b0000000}},
    {"sra",   {"sra",   Format::R,  0b0110011, 0b101, 0b0100000}},
    {"srl",   {"srl",   Format::R,  0b0110011, 0b101, 0b0000000}},
    {"sub",   {"sub",   Format::R,  0b0110011, 0b000, 0b0100000}},
    {"xor",   {"xor",   Format::R,  0b0110011, 0b100, 0b0000000}},
    {"mul",   {"mul",   Format::R,  0b0110011, 0b000, 0b0000001}},
    {"div",   {"div",   Format::R,  0b0110011, 0b100, 0b0000001}},
    {"rem",   {"rem",   Format::R,  0b0110011, 0b110, 0b0000001}},
    {"addi",  {"addi",  Format::I,  0b0010011, 0b000, 0}},
    {"andi",  {"andi",  Format::I,  0b0010011, 0b111, 0}},
    {"ori",   {"ori",   Format::I,  0b0010011, 0b110, 0}},
    {"lb",    {"lb",    Format::I,  0b0000011, 0b000, 0}},
    {"lh",    {"lh",    Format::I,  0b0000011, 0b001, 0}},
    {"lw",    {"lw",    Format::I,  0b0000011, 0b010, 0}},
    {"ld",    {"ld",    Format::I,  0b0000011, 0b011, 0}},
    {"jalr",  {"jalr",  Format::I,  0b1100111, 0b000, 0}},
    {"sb",    {"sb",    Format::S,  0b0100011, 0b000, 0}},
    {"sh",    {"sh",    Format::S,  0b0100011, 0b001, 0}},
    {"sw",    {"sw",    Format::S,  0b0100011, 0b010, 0}},
    {"sd",    {"sd",    Format::S,  0b0100011, 0b011, 0}},
    {"beq",   {"beq",   Format::SB, 0b1100011, 0b000, 0}},
    {"bne",   {"bne",   Format::SB, 0b1100011, 0b001, 0}},
    {"bge",   {"bge",   Format::SB, 0b1100011, 0b101, 0}},
    {"blt",   {"blt",   Format::SB, 0b1100011, 0b100, 0}},
    {"auipc", {"auipc", Format::U,  0b0010111, 0,     0}},
    {"lui",   {"lui",   Format::U,  0b0110111, 0,     0}},
    {"jal",   {"jal",   Format::UJ, 0b1101111, 0,     0}}
};

// Function to parse register number
int parseRegister(const std::string& reg) {
    if (reg[0] == 'x') {
        return stoi(reg.substr(1));
    }
    return -1;
}

// Function to parse immediate value
int parseImmediate(const std::string& imm) {
    return stoi(imm);
}

int main(){
    return 0;
}
