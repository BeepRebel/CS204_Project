#include <bits/stdc++.h>
using namespace std;

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

        // Process data segment after code segment
        assembleData();
    }

    string processInstruction(const string& line, uint32_t currentAddress)
{
    vector<string> tokens = split(line);
    if (tokens.empty()) {
        throw runtime_error("Empty instruction");
    }
    
    string mnemonic = tokens[0];
    uint32_t encodedInstruction = 0;
    
    try {
        // R-type instructions
        if (mnemonic == "add" || mnemonic == "and" || mnemonic == "or" || mnemonic == "sll" || 
            mnemonic == "slt" || mnemonic == "sra" || mnemonic == "srl" || mnemonic == "sub" || 
            mnemonic == "xor" || mnemonic == "mul" || mnemonic == "div" || mnemonic == "rem") {
            if (tokens.size() != 4) {
                throw runtime_error(mnemonic + " instruction requires 3 register operands");
            }
            uint32_t rd = parseRegister(tokens[1]);
            uint32_t rs1 = parseRegister(tokens[2]);
            uint32_t rs2 = parseRegister(tokens[3]);
            
            uint32_t funct3 = (mnemonic == "sll") ? 1 : (mnemonic == "slt") ? 2 : (mnemonic == "xor") ? 4 :
                              (mnemonic == "srl" || mnemonic == "sra") ? 5 : (mnemonic == "or") ? 6 :
                              (mnemonic == "and") ? 7 : 0;
            
            uint32_t funct7 = (mnemonic == "sub" || mnemonic == "sra") ? 0x20 : 
                              (mnemonic == "mul") ? 0x01 : (mnemonic == "div") ? 0x01 : 
                              (mnemonic == "rem") ? 0x01 : 0;
            
            encodedInstruction = encodeRType("0x33", rd, rs1, rs2, funct3, funct7);
        }
        
        // I-type instructions
        else if (mnemonic == "addi" || mnemonic == "andi" || mnemonic == "ori" || 
                 mnemonic == "lb" || mnemonic == "ld" || mnemonic == "lh" || mnemonic == "lw" || mnemonic == "jalr") {
            if (tokens.size() != 4) {
                throw runtime_error(mnemonic + " instruction requires 2 registers and an immediate");
            }
            uint32_t rd = parseRegister(tokens[1]);
            uint32_t rs1 = parseRegister(tokens[2]);
            int32_t imm = parseImmediate(tokens[3]);
            
            uint32_t funct3 = (mnemonic == "andi") ? 7 : (mnemonic == "ori") ? 6 : 
                              (mnemonic == "lb") ? 0 : (mnemonic == "lh") ? 1 :
                              (mnemonic == "lw") ? 2 : (mnemonic == "ld") ? 3 : 0;
            
            encodedInstruction = encodeIType(mnemonic == "jalr" ? "0x67" : "0x13", rd, rs1, imm, funct3);
        }
        
        // S-type instructions
        else if (mnemonic == "sb" || mnemonic == "sw" || mnemonic == "sd" || mnemonic == "sh") {
            if (tokens.size() != 3) {
                throw runtime_error(mnemonic + " instruction requires a register and memory address");
            }
            uint32_t rs2 = parseRegister(tokens[1]);
            int32_t imm = parseImmediate(tokens[2]);
            uint32_t rs1 = parseRegister(tokens[2].substr(tokens[2].find('(') + 1, tokens[2].find(')') - tokens[2].find('(') - 1));
            
            uint32_t funct3 = (mnemonic == "sb") ? 0 : (mnemonic == "sh") ? 1 : 
                              (mnemonic == "sw") ? 2 : (mnemonic == "sd") ? 3 : 0;
            
            encodedInstruction = encodeSType("0x23", rs1, rs2, imm, funct3);
        }
        
        // SB-type (Branch) instructions
        else if (mnemonic == "beq" || mnemonic == "bne" || mnemonic == "bge" || mnemonic == "blt") {
            if (tokens.size() != 4) {
                throw runtime_error(mnemonic + " instruction requires 2 registers and a target");
            }
            uint32_t rs1 = parseRegister(tokens[1]);
            uint32_t rs2 = parseRegister(tokens[2]);
            int32_t imm = parseImmediate(tokens[3], currentAddress, true);
            
            uint32_t funct3 = (mnemonic == "beq") ? 0 : (mnemonic == "bne") ? 1 : 
                              (mnemonic == "blt") ? 4 : (mnemonic == "bge") ? 5 : 0;
            
            encodedInstruction = encodeSBType("0x63", rs1, rs2, imm, funct3);
        }
        
        // U-type instructions
        else if (mnemonic == "auipc" || mnemonic == "lui") {
            if (tokens.size() != 3) {
                throw runtime_error(mnemonic + " instruction requires a register and an immediate");
            }
            uint32_t rd = parseRegister(tokens[1]);
            int32_t imm = parseImmediate(tokens[2]);
            
            encodedInstruction = encodeUType(mnemonic == "auipc" ? "0x17" : "0x37", rd, imm);
        }
        
        // UJ-type (Jump) instruction
        else if (mnemonic == "jal") {
            if (tokens.size() != 3) {
                throw runtime_error("jal instruction requires a register and a target");
            }
            uint32_t rd = parseRegister(tokens[1]);
            int32_t imm = parseImmediate(tokens[2], currentAddress, true);
            
            encodedInstruction = encodeUJType("0x6F", rd, imm);
        }
        
        else {
            throw runtime_error("Unsupported instruction: " + mnemonic);
        }
    } catch (const exception& e) {
        throw runtime_error(string("Error processing instruction: ") + e.what());
    }
    
    // Return encoded instruction as hexadecimal string
    return "0x" + to_string_hex(encodedInstruction);
}


    // Helper function to convert integer to hexadecimal string
    string to_string_hex(uint32_t value)
    {
        stringstream ss;
        ss << hex << setw(8) << setfill('0') << value;
        return ss.str();
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

                    string output = decToHex(address) + " " + decToHex(val); // building final output
                    outputLines.push_back(output);                           // adding output
                    address += size;                                         // updating data Address
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

                    string output = decToHex(address) + " " + decToHex(val); // building final output
                    outputLines.push_back(output);                           // adding output
                    address += size;                                         // updating data Address
                }
            }
        }
    }

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
