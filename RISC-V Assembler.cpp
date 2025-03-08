#include <bits/stdc++.h>
using namespace std;
// First, define a structure to hold instruction information
struct InstructionInfo {
    enum InstructionType {
        R_TYPE,
        I_TYPE,
        S_TYPE,
        SB_TYPE,
        U_TYPE,
        UJ_TYPE
    };
    
    InstructionType type;
    string opcode;
    uint32_t funct3;
    uint32_t funct7;
    int operandCount;
};

// Define the instruction table
unordered_map<string, InstructionInfo> instructionTable;

// Initialize the instruction table
void initInstructionTable() {
    // R-type instructions
    // Base RV32I instructions
    instructionTable["add"] = {InstructionInfo::R_TYPE, "0x33", 0, 0x00, 3};
    instructionTable["sub"] = {InstructionInfo::R_TYPE, "0x33", 0, 0x20, 3};
    instructionTable["sll"] = {InstructionInfo::R_TYPE, "0x33", 1, 0x00, 3};
    instructionTable["slt"] = {InstructionInfo::R_TYPE, "0x33", 2, 0x00, 3};
    instructionTable["sltu"] = {InstructionInfo::R_TYPE, "0x33", 3, 0x00, 3};
    instructionTable["xor"] = {InstructionInfo::R_TYPE, "0x33", 4, 0x00, 3};
    instructionTable["srl"] = {InstructionInfo::R_TYPE, "0x33", 5, 0x00, 3};
    instructionTable["sra"] = {InstructionInfo::R_TYPE, "0x33", 5, 0x20, 3};
    instructionTable["or"] = {InstructionInfo::R_TYPE, "0x33", 6, 0x00, 3};
    instructionTable["and"] = {InstructionInfo::R_TYPE, "0x33", 7, 0x00, 3};
    
    // M extension instructions
    instructionTable["mul"] = {InstructionInfo::R_TYPE, "0x33", 0, 0x01, 3};
    instructionTable["mulh"] = {InstructionInfo::R_TYPE, "0x33", 1, 0x01, 3};
    instructionTable["mulhsu"] = {InstructionInfo::R_TYPE, "0x33", 2, 0x01, 3};
    instructionTable["mulhu"] = {InstructionInfo::R_TYPE, "0x33", 3, 0x01, 3};
    instructionTable["div"] = {InstructionInfo::R_TYPE, "0x33", 4, 0x01, 3};
    instructionTable["divu"] = {InstructionInfo::R_TYPE, "0x33", 5, 0x01, 3};
    instructionTable["rem"] = {InstructionInfo::R_TYPE, "0x33", 6, 0x01, 3};
    instructionTable["remu"] = {InstructionInfo::R_TYPE, "0x33", 7, 0x01, 3};
    
    // I-type instructions
    // Base ALU operations
    instructionTable["addi"] = {InstructionInfo::I_TYPE, "0x13", 0, 0, 3};
    instructionTable["slti"] = {InstructionInfo::I_TYPE, "0x13", 2, 0, 3};
    instructionTable["sltiu"] = {InstructionInfo::I_TYPE, "0x13", 3, 0, 3};
    instructionTable["xori"] = {InstructionInfo::I_TYPE, "0x13", 4, 0, 3};
    instructionTable["ori"] = {InstructionInfo::I_TYPE, "0x13", 6, 0, 3};
    instructionTable["andi"] = {InstructionInfo::I_TYPE, "0x13", 7, 0, 3};
    instructionTable["slli"] = {InstructionInfo::I_TYPE, "0x13", 1, 0, 3};
    instructionTable["srli"] = {InstructionInfo::I_TYPE, "0x13", 5, 0, 3};
    instructionTable["srai"] = {InstructionInfo::I_TYPE, "0x13", 5, 0x20, 3};
    
    // Load instructions (I-type)
    instructionTable["lb"] = {InstructionInfo::I_TYPE, "0x03", 0, 0, 2};
    instructionTable["lh"] = {InstructionInfo::I_TYPE, "0x03", 1, 0, 2};
    instructionTable["lw"] = {InstructionInfo::I_TYPE, "0x03", 2, 0, 2};
    instructionTable["lbu"] = {InstructionInfo::I_TYPE, "0x03", 4, 0, 2};
    instructionTable["lhu"] = {InstructionInfo::I_TYPE, "0x03", 5, 0, 2};
    // RV64I load instruction
    instructionTable["ld"] = {InstructionInfo::I_TYPE, "0x03", 3, 0, 2};
    instructionTable["lwu"] = {InstructionInfo::I_TYPE, "0x03", 6, 0, 2};
    
    // Jump instructions (I-type)
    instructionTable["jalr"] = {InstructionInfo::I_TYPE, "0x67", 0, 0, 3};
    
    // S-type instructions (Store)
    instructionTable["sb"] = {InstructionInfo::S_TYPE, "0x23", 0, 0, 2};
    instructionTable["sh"] = {InstructionInfo::S_TYPE, "0x23", 1, 0, 2};
    instructionTable["sw"] = {InstructionInfo::S_TYPE, "0x23", 2, 0, 2};
    // RV64I store instruction
    instructionTable["sd"] = {InstructionInfo::S_TYPE, "0x23", 3, 0, 2};
    
    // SB-type instructions (branches)
    instructionTable["beq"] = {InstructionInfo::SB_TYPE, "0x63", 0, 0, 3};
    instructionTable["bne"] = {InstructionInfo::SB_TYPE, "0x63", 1, 0, 3};
    instructionTable["blt"] = {InstructionInfo::SB_TYPE, "0x63", 4, 0, 3};
    instructionTable["bge"] = {InstructionInfo::SB_TYPE, "0x63", 5, 0, 3};
    instructionTable["bltu"] = {InstructionInfo::SB_TYPE, "0x63", 6, 0, 3};
    instructionTable["bgeu"] = {InstructionInfo::SB_TYPE, "0x63", 7, 0, 3};
    
    // U-type instructions
    instructionTable["lui"] = {InstructionInfo::U_TYPE, "0x37", 0, 0, 2};
    instructionTable["auipc"] = {InstructionInfo::U_TYPE, "0x17", 0, 0, 2};
    
    // UJ-type instructions
    instructionTable["jal"] = {InstructionInfo::UJ_TYPE, "0x6F", 0, 0, 2};
}
   
class RISCVAssembler
{
private:
    // Symbol Table: Stores label names and their corresponding memory addresses
    unordered_map<string, uint32_t> symbolTable;
    
    // Track which segment each label belongs to
    unordered_map<string, string> labelSegment;

    // Directive sizes map: maps directives to their byte sizes
    unordered_map<string, int> directivesSizes;

    // Memory Segment Definitions
    uint32_t codeSegmentStart = 0x00000000; // Start of executable code
    uint32_t dataSegmentStart = 0x10000000; // Start of data segment (0x10000000 = 268435456)
    uint32_t heapStart = 0x10008000;        // Heap memory start
    uint32_t stackStart = 0x7FFFFFD0;       // Stack memory start

    // Stores the generated instructions with their addresses
    vector<pair<string, string>> instructions;

    // Stores data segment information
    vector<pair<string, string>> dataSegment;

    // Store raw data lines for processing
    vector<string> dataLines;

    // Store output lines (combined code and data)
    vector<string> outputLines;

    // Converts register name (e.g., "x1") to register number
    uint32_t parseRegister(const string &reg)
    {
        if (reg[0] != 'x')
            throw runtime_error("Invalid register format: " + reg);
        
        int regNum = stoi(reg.substr(1)); // Extract number after 'x'
        
        // Check if register number is valid (0-31 for RISC-V)
        if (regNum < 0 || regNum > 31)
            throw runtime_error("Invalid register number: " + reg);
            
        return regNum;
    }

    // Parses immediate values
    int32_t parseImmediate(const string &imm, uint32_t currentAddress = 0, bool isPCRelative = false)
    {
        // Check if the immediate is a previously defined label
        if (symbolTable.find(imm) != symbolTable.end())
        {
            int32_t targetAddress = symbolTable[imm];
            // If PC-relative (for branch/jump instructions), calculate the offset
            if (isPCRelative)
            {
                return targetAddress - currentAddress;
            }
            return targetAddress;
        }

        // Check for hexadecimal number
        if (imm.substr(0, 2) == "0x")
        {
            return stoi(imm, nullptr, 16);
        }

        // Parse as decimal number
        return stoi(imm);
    }

    // Encoding functions for different RISC-V instruction formats
    uint32_t encodeRType(const string &opcode, uint32_t rd, uint32_t rs1,
                         uint32_t rs2, uint32_t funct3, uint32_t funct7)
    {
        return (funct7 << 25)                // Func7 bits (top 7 bits)
               | (rs2 << 20)                 // Source register 2 (bits 20-24)
               | (rs1 << 15)                 // Source register 1 (bits 15-19)
               | (funct3 << 12)              // Func3 bits (bits 12-14)
               | (rd << 7)                   // Destination register (bits 7-11)
               | stoul(opcode, nullptr, 16); // Opcode (bottom 7 bits)
    }

    uint32_t encodeIType(const string &opcode, uint32_t rd, uint32_t rs1,
                         int32_t imm, uint32_t funct3)
    {
        // Ensure immediate is 12-bit and sign-extended properly
        int32_t signedImm = imm & 0xFFF;
        
        return ((signedImm & 0xFFF) << 20)   // Immediate value (bits 20-31)
               | (rs1 << 15)                 // Source register 1 (bits 15-19)
               | (funct3 << 12)              // Func3 bits (bits 12-14)
               | (rd << 7)                   // Destination register (bits 7-11)
               | stoul(opcode, nullptr, 16); // Opcode (bottom 7 bits)
    }

    uint32_t encodeSType(const string &opcode, uint32_t rs1, uint32_t rs2,
                         int32_t imm, uint32_t funct3)
    {
        // Split 12-bit immediate into two parts for encoding
        imm = imm & 0xFFF; // Ensure imm is 12-bit
        uint32_t imm11_5 = (imm & 0xFE0) >> 5; // Upper 7 bits
        uint32_t imm4_0 = imm & 0x1F;          // Lower 5 bits

        return (imm11_5 << 25)               // Immediate [11:5] (bits 25-31)
               | (rs2 << 20)                 // Source register 2 (bits 20-24)
               | (rs1 << 15)                 // Source register 1 (bits 15-19)
               | (funct3 << 12)              // Func3 bits (bits 12-14)
               | (imm4_0 << 7)               // Immediate [4:0] (bits 7-11)
               | stoul(opcode, nullptr, 16); // Opcode (bottom 7 bits)
    }

    // SB-Type instructions (branches)
    uint32_t encodeSBType(const string &opcode, uint32_t rs1, uint32_t rs2,
                          int32_t imm, uint32_t funct3)
    {
        // Ensure branch offset is aligned
        if (imm % 2 != 0)
            throw runtime_error("Branch target must be 2-byte aligned");
            
        // For branch instructions, we divide the offset by 2 to get the 
        // correct 13-bit signed offset (including the implicit 0 bit)
        imm = imm >> 1;
        
        // Extract bits for encoding
        uint32_t imm12 = (imm & 0x1000) >> 12;   // Bit 12
        uint32_t imm11 = (imm & 0x800) >> 11;    // Bit 11
        uint32_t imm10_5 = (imm & 0x7E0) >> 5;   // Bits 10-5
        uint32_t imm4_1 = (imm & 0x1E) >> 1;     // Bits 4-1
        
        return (imm12 << 31)                 // Bit 12 (bit 31)
               | (imm10_5 << 25)             // Bits 10-5 (bits 25-30)
               | (rs2 << 20)                 // Source register 2 (bits 20-24)
               | (rs1 << 15)                 // Source register 1 (bits 15-19)
               | (funct3 << 12)              // Func3 bits (bits 12-14)
               | (imm4_1 << 8)               // Bits 4-1 (bits 8-11)
               | (imm11 << 7)                // Bit 11 (bit 7)
               | stoul(opcode, nullptr, 16); // Opcode (bottom 7 bits)
    }
    
    // U-Type instructions (lui, auipc)
    uint32_t encodeUType(const string &opcode, uint32_t rd, int32_t imm)
    {
        // U-type uses the upper 20 bits of immediate
        uint32_t imm31_12 = (imm & 0xFFFFF000);
        
        return imm31_12                      // Upper 20 bits (bits 12-31)
               | (rd << 7)                   // Destination register (bits 7-11)
               | stoul(opcode, nullptr, 16); // Opcode (bottom 7 bits)
    }
    
    // UJ-Type instructions (jal)
    uint32_t encodeUJType(const string &opcode, uint32_t rd, int32_t imm)
    {
        // Check that jump target is 2-byte aligned
        if (imm % 2 != 0)
            throw runtime_error("Jump target must be 2-byte aligned");
            
        // For jump instructions, we divide the offset by 2
        imm = imm >> 1;
        
        // Extract bits for encoding
        uint32_t imm20 = (imm & 0x100000) >> 20;     // Bit 20
        uint32_t imm19_12 = (imm & 0xFF000) >> 12;   // Bits 19-12
        uint32_t imm11 = (imm & 0x800) >> 11;        // Bit 11
        uint32_t imm10_1 = (imm & 0x7FE) >> 1;       // Bits 10-1
        
        return (imm20 << 31)                 // Bit 20 (bit 31)
               | (imm10_1 << 21)             // Bits 10-1 (bits 21-30)
               | (imm11 << 20)               // Bit 11 (bit 20)
               | (imm19_12 << 12)            // Bits 19-12 (bits 12-19)
               | (rd << 7)                   // Destination register (bits 7-11)
               | stoul(opcode, nullptr, 16); // Opcode (bottom 7 bits)
    }

    // Split a string into tokens
    vector<string> split(const string &line)
    {
        vector<string> tokens;
        stringstream ss(line);
        string token;

        while (ss >> token)
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    // Convert decimal to hexadecimal string representation
    string decToHex(long long dec)
    {
        stringstream ss;
        ss << "0x" << hex << dec;
        return ss.str();
    }

    // FIRST PASS: Collect Symbol Table
    void firstPass(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Failed to open input file: " + filename);
        }
        
        string line;
        uint32_t currentAddress = codeSegmentStart;
        string currentSegment = ".text";

        while (getline(file, line))
        {
            // Remove comments
            line = regex_replace(line, regex(";.*$"), "");

            // Skip empty lines
            if (line.empty() || all_of(line.begin(), line.end(), ::isspace))
                continue;

            // Handle segment directives
            if (line.find(".text") != string::npos)
            {
                currentSegment = ".text";
                currentAddress = codeSegmentStart;
                continue;
            }
            if (line.find(".data") != string::npos)
            {
                currentSegment = ".data";
                currentAddress = dataSegmentStart;
                continue;
            }

            // Identify and store labels
            if (line.find(':') != string::npos)
            {
                string label = line.substr(0, line.find(':'));
                symbolTable[label] = currentAddress;
                labelSegment[label] = currentSegment; // Track which segment the label belongs to

                // Remove label from line
                line = line.substr(line.find(':') + 1);
                line = regex_replace(line, regex("^\\s+"), "");
            }

            // Store data lines for second pass
            if (currentSegment == ".data" && !line.empty())
            {
                dataLines.push_back(line);
                // Data address increment will be calculated in the assembleData function
                
                // Calculate address increment for data directives
                vector<string> tokens = split(line);
                if (!tokens.empty() && directivesSizes.find(tokens[0]) != directivesSizes.end())
                {
                    int size = directivesSizes[tokens[0]];
                    if (tokens[0] == ".asciiz" && tokens.size() > 1) {
                        // For .asciiz, the size is the string length + 1 (null terminator)
                        string str = tokens[1];
                        // Remove quotes
                        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
                            str = str.substr(1, str.size() - 2);
                        }
                        currentAddress += (str.size() + 1) * size;
                    } else {
                        // For other directives, the size is the size of each element times the number of elements
                        currentAddress += size * (tokens.size() - 1);
                    }
                    
                    // Ensure proper alignment
                    if (size > 1) {
                        currentAddress = (currentAddress + size - 1) & ~(size - 1);
                    }
                }
            }
            // Increment address for text segment instructions
            else if (currentSegment == ".text" && !line.empty())
            {
                currentAddress += 4; // Each instruction is 4 bytes
            }
        }
    }

    // SECOND PASS: Generate Machine Code
// Process instruction implementation
string processInstruction(const string& line, uint32_t currentAddress)
{
    vector<string> tokens = split(line);
    if (tokens.empty()) {
        throw runtime_error("Empty instruction");
    }
    
    string mnemonic = tokens[0];
    uint32_t encodedInstruction = 0;
    string decodedBinary = "";
    
    try {
        // Check if this is a known instruction
        if (instructionTable.find(mnemonic) == instructionTable.end()) {
            throw runtime_error("Unknown instruction: " + mnemonic);
        }
        
        // Get instruction info
        InstructionInfo info = instructionTable[mnemonic];
        
        // Check if we have the correct number of operands
        if (tokens.size() - 1 != info.operandCount) {
            throw runtime_error(mnemonic + " instruction requires " + 
                              to_string(info.operandCount) + " operands");
        }
        
        // Process based on instruction type
        switch (info.type) {
            case InstructionInfo::R_TYPE: {
                uint32_t rd = parseRegister(tokens[1]);
                uint32_t rs1 = parseRegister(tokens[2]);
                uint32_t rs2 = parseRegister(tokens[3]);
                encodedInstruction = encodeRType(info.opcode, rd, rs1, rs2, info.funct3, info.funct7);
                
                // Create binary representation for comment
                string funct7Bin = bitset<7>(info.funct7).to_string();
                string rs2Bin = bitset<5>(rs2).to_string();
                string rs1Bin = bitset<5>(rs1).to_string();
                string funct3Bin = bitset<3>(info.funct3).to_string();
                string rdBin = bitset<5>(rd).to_string();
                string opcodeBin = bitset<7>(stoul(info.opcode, nullptr, 16)).to_string();
                decodedBinary = funct7Bin + "-" + rs2Bin + "-" + rs1Bin + "-" + funct3Bin + "-" + rdBin + "-" + opcodeBin;
                break;
            }
            case InstructionInfo::I_TYPE: {
                uint32_t rd = parseRegister(tokens[1]);
                uint32_t rs1 = 0;
                int32_t imm = 0;
                
                // Handle special case for load instructions
                if (mnemonic == "lb" || mnemonic == "lh" || mnemonic == "lw" || 
                    mnemonic == "lbu" || mnemonic == "lhu" || mnemonic == "ld" || mnemonic == "lwu") {
                    // Parse memory operand of format: imm(rs1)
                    string memOp = tokens[2];
                    size_t openParen = memOp.find('(');
                    size_t closeParen = memOp.find(')');
                    
                    if (openParen == string::npos || closeParen == string::npos) {
                        throw runtime_error("Invalid memory addressing format: " + memOp);
                    }
                    
                    string immStr = memOp.substr(0, openParen);
                    string rs1Str = memOp.substr(openParen + 1, closeParen - openParen - 1);
                    
                    imm = parseImmediate(immStr);
                    rs1 = parseRegister(rs1Str);
                } else {
                    rs1 = parseRegister(tokens[2]);
                    imm = parseImmediate(tokens[3]);
                }
                
                encodedInstruction = encodeIType(info.opcode, rd, rs1, imm, info.funct3);
                
                // Create binary representation for comment
                string immBin = bitset<12>(imm & 0xFFF).to_string();
                string rs1Bin = bitset<5>(rs1).to_string();
                string funct3Bin = bitset<3>(info.funct3).to_string();
                string rdBin = bitset<5>(rd).to_string();
                string opcodeBin = bitset<7>(stoul(info.opcode, nullptr, 16)).to_string();
                decodedBinary = immBin + "-" + rs1Bin + "-" + funct3Bin + "-" + rdBin + "-" + opcodeBin;
                break;
            }
            case InstructionInfo::S_TYPE: {
                uint32_t rs2 = parseRegister(tokens[1]);
                
                // Parse memory operand of format: imm(rs1)
                string memOp = tokens[2];
                size_t openParen = memOp.find('(');
                size_t closeParen = memOp.find(')');
                
                if (openParen == string::npos || closeParen == string::npos) {
                    throw runtime_error("Invalid memory addressing format: " + memOp);
                }
                
                string immStr = memOp.substr(0, openParen);
                string rs1Str = memOp.substr(openParen + 1, closeParen - openParen - 1);
                
                int32_t imm = parseImmediate(immStr);
                uint32_t rs1 = parseRegister(rs1Str);
                
                encodedInstruction = encodeSType(info.opcode, rs1, rs2, imm, info.funct3);
                
                // Create binary representation for comment
                uint32_t imm11_5 = (imm & 0xFE0) >> 5;
                uint32_t imm4_0 = imm & 0x1F;
                string imm11_5Bin = bitset<7>(imm11_5).to_string();
                string rs2Bin = bitset<5>(rs2).to_string();
                string rs1Bin = bitset<5>(rs1).to_string();
                string funct3Bin = bitset<3>(info.funct3).to_string();
                string imm4_0Bin = bitset<5>(imm4_0).to_string();
                string opcodeBin = bitset<7>(stoul(info.opcode, nullptr, 16)).to_string();
                decodedBinary = imm11_5Bin + "-" + rs2Bin + "-" + rs1Bin + "-" + funct3Bin + "-" + imm4_0Bin + "-" + opcodeBin;
                break;
            }
            case InstructionInfo::SB_TYPE: {
                uint32_t rs1 = parseRegister(tokens[1]);
                uint32_t rs2 = parseRegister(tokens[2]);
                int32_t imm = parseImmediate(tokens[3], currentAddress, true);
                encodedInstruction = encodeSBType(info.opcode, rs1, rs2, imm, info.funct3);
                
                // Extract bits for creating binary representation
                imm = imm >> 1; // Already shifted in the encode function
                uint32_t imm12 = (imm & 0x1000) >> 12;
                uint32_t imm11 = (imm & 0x800) >> 11;
                uint32_t imm10_5 = (imm & 0x7E0) >> 5;
                uint32_t imm4_1 = (imm & 0x1E) >> 1;
                
                string imm12_11Bin = bitset<1>(imm12).to_string() + bitset<1>(imm11).to_string();
                string imm10_5Bin = bitset<6>(imm10_5).to_string();
                string rs2Bin = bitset<5>(rs2).to_string();
                string rs1Bin = bitset<5>(rs1).to_string();
                string funct3Bin = bitset<3>(info.funct3).to_string();
                string imm4_1_0Bin = bitset<4>(imm4_1).to_string() + "0";
                string opcodeBin = bitset<7>(stoul(info.opcode, nullptr, 16)).to_string();
                decodedBinary = imm12_11Bin + "-" + imm10_5Bin + "-" + rs2Bin + "-" + rs1Bin + "-" + funct3Bin + "-" + imm4_1_0Bin + "-" + opcodeBin;
                break;
            }
            case InstructionInfo::U_TYPE: {
                uint32_t rd = parseRegister(tokens[1]);
                int32_t imm = parseImmediate(tokens[2]);
                encodedInstruction = encodeUType(info.opcode, rd, imm);
                
                // Create binary representation for comment
                string immBin = bitset<20>((imm & 0xFFFFF000) >> 12).to_string();
                string rdBin = bitset<5>(rd).to_string();
                string opcodeBin = bitset<7>(stoul(info.opcode, nullptr, 16)).to_string();
                decodedBinary = immBin + "-" + rdBin + "-" + opcodeBin;
                break;
            }
            case InstructionInfo::UJ_TYPE: {
                uint32_t rd = parseRegister(tokens[1]);
                int32_t imm = parseImmediate(tokens[2], currentAddress, true);
                encodedInstruction = encodeUJType(info.opcode, rd, imm);
                
                // Extract bits for binary representation
                imm = imm >> 1;
                uint32_t imm20 = (imm & 0x100000) >> 20;
                uint32_t imm19_12 = (imm & 0xFF000) >> 12;
                uint32_t imm11 = (imm & 0x800) >> 11;
                uint32_t imm10_1 = (imm & 0x7FE) >> 1;
                
                string immBin = bitset<1>(imm20).to_string() + 
                                bitset<8>(imm19_12).to_string() + 
                                bitset<1>(imm11).to_string() + 
                                bitset<10>(imm10_1).to_string();
                string rdBin = bitset<5>(rd).to_string();
                string opcodeBin = bitset<7>(stoul(info.opcode, nullptr, 16)).to_string();
                decodedBinary = immBin + "-" + rdBin + "-" + opcodeBin;
                break;
            }
            default:
                throw runtime_error("Unsupported instruction type for: " + mnemonic);
        }
        
    } catch (const exception& e) {
        throw runtime_error(string("Error processing instruction: ") + e.what());
    }
    
    // Return encoded instruction as hexadecimal string along with the original instruction and binary decoding
    return "0x" + to_string_hex(encodedInstruction) + " , " + line + " # " + decodedBinary + "-NULL";
}

// Process data segment
void assembleData()
{
    vector<string> words;                 // stores tokens of a given command
    int size;                             // stores size of data element to be added
    long long address = dataSegmentStart; // Starting address of data
    long long val;                        // Temporarily stores value from a command

    for (auto code : dataLines)
    {
        words = split(code); // split command into individual tokens
        
        if (words.empty()) continue;

        if (directivesSizes.find(words[0]) != directivesSizes.end())
        {
            size = directivesSizes[words[0]]; // extract size of data to be stored
            
            // Ensure proper alignment for multi-byte data
            if (size > 1) {
                address = (address + size - 1) & ~(size - 1);
            }
        }
        else // Error handling
        {
            cerr << "Error at .data segment" << endl;
            cerr << "Line : " << code << endl;
            cerr << "Unknown directive: " << words[0] << endl;
            exit(-1);
        }

        if (words[0] == ".asciiz") // for .asciiz
        {
            if (words.size() < 2) {
                cerr << "Error: .asciiz directive requires a string argument" << endl;
                cerr << "Line: " << code << endl;
                exit(-1);
            }
            
            string s = words[1];
            // Remove quotes if present
            if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
                s = s.substr(1, s.size() - 2);
            }

            for (int i = 0; i < s.length(); i++)
            {
                val = s[i]; // get ASCII value

                // Error handling
                if (val < 0 || val > 255)
                {
                    cerr << "Error at .data segment" << endl;
                    cerr << "Line : " << code << endl;
                    cerr << "Value out of bounds: " << val << endl;
                    exit(-1);
                }

                // Modified output format
                string output = decToHex(address) + " " + decToHex(val);
                outputLines.push_back(output);
                address += size;
            }

            // Add null terminator for .asciiz
            outputLines.push_back(decToHex(address) + " " + decToHex(0));
            address += size;
        }
        else // for other directives
        {
            if (words.size() < 2) {
                cerr << "Error: " << words[0] << " directive requires at least one argument" << endl;
                cerr << "Line: " << code << endl;
                exit(-1);
            }
            
            for (int i = 1; i < words.size(); i++)
            {
                try {
                    val = stoll(words[i]); // Extracting values
                } catch (const exception& e) {
                    cerr << "Error parsing data value: " << words[i] << endl;
                    cerr << "Line: " << code << endl;
                    exit(-1);
                }

                // Error handling for value ranges
                int64_t min_val = -(static_cast<int64_t>(1) << (size * 8 - 1));
                int64_t max_val = (static_cast<int64_t>(1) << (size * 8 - 1)) - 1;
                
                if (val < min_val || val > max_val)
                {
                    cerr << "Error at .data segment" << endl;
                    cerr << "Line : " << code << endl;
                    cerr << "Value out of bounds for " << words[0] << ": " << val << endl;
                    cerr << "Valid range: " << min_val << " to " << max_val << endl;
                    exit(-1);
                }

                // Modified output format
                string output = decToHex(address) + " " + decToHex(val);
                outputLines.push_back(output);
                address += size;
            }
        }
    }
}

// SECOND PASS: Generate Machine Code
void secondPass(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Failed to open input file: " + filename);
    }
    
    string line;
    uint32_t currentAddress = codeSegmentStart;
    string currentSegment = ".text";

    while (getline(file, line))
    {
        // Remove comments and skip empty lines
        line = regex_replace(line, regex(";.*$"), "");

        if (line.empty() || all_of(line.begin(), line.end(), ::isspace))
            continue;
            
        // Handle segment directives
        if (line.find(".text") != string::npos)
        {
            currentSegment = ".text";
            currentAddress = codeSegmentStart;
            continue;
        }
        if (line.find(".data") != string::npos)
        {
            currentSegment = ".data";
            currentAddress = dataSegmentStart;
            continue;
        }

        // Remove labels
        if (line.find(':') != string::npos)
        {
            line = line.substr(line.find(':') + 1);
            line = regex_replace(line, regex("^\\s+"), "");
        }

        // Process text segment instructions
        if (currentSegment == ".text" && !line.empty())
        {
            try {
                string machineCode = processInstruction(line, currentAddress);
                outputLines.push_back(decToHex(currentAddress) + " " + machineCode);
                currentAddress += 4;
            } catch (const exception& e) {
                cerr << "Error at address " << decToHex(currentAddress) << ": " << e.what() << endl;
                cerr << "Line: " << line << endl;
                exit(-1);
            }
        }
    }

   
    // Add termination code after text segment
    outputLines.push_back(decToHex(currentAddress));
    // Process data segment after code segment
    assembleData();
}
    // Helper function to convert integer to hexadecimal string
    string to_string_hex(uint32_t value)
    {
        stringstream ss;
        ss << hex << setw(8) << setfill('0') << value;
        return ss.str();
    }

    // Process data segment
    // Process data segment
 // Initialize the directives sizes map
    void initDirectivesSizes()
    {
        directivesSizes[".byte"] = 1;   // 1 byte
        directivesSizes[".half"] = 2;   // 2 bytes
        directivesSizes[".word"] = 4;   // 4 bytes
        directivesSizes[".asciiz"] = 1; // 1 byte per character
        directivesSizes[".space"] = 1;  // 1 byte per space
    }

public:
    // Constructor
    RISCVAssembler()
    {
        initDirectivesSizes();
        initInstructionTable();
    }

    // Main assembler method
    void assemble(const string &inputFile, const string &outputFile)
    {
        try {
            // Collect symbol information in first pass
            firstPass(inputFile);

            // Generate machine code in second pass
            secondPass(inputFile);

            // Write machine code to output file
            ofstream outFile(outputFile);
            if (!outFile.is_open()) {
                throw runtime_error("Failed to open output file: " + outputFile);
            }

            for (const auto &line : outputLines)
            {
                outFile << line << endl;
            }
            
            cout << "Assembly successful. Output written to " << outputFile << endl;
        } catch (const exception& e) {
            cerr << "Assembly failed: " << e.what() << endl;
            exit(-1);
        }
    }
};

int main()
{

    RISCVAssembler assembler;
    assembler.assemble("input.asm", "output.mc");

    return 0;
}
