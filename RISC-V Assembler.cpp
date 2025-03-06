// RISC-V Assembler
#include <bits/stdc++.h>
using namespace std;

class RISCVAssembler
{
private:
    // Symbol Table: Stores label names and their corresponding memory addresses
    // This is crucial for resolving labels in branch and jump instructions
    unordered_map<string, uint32_t> symbolTable;

    // Memory Segment Definitions (Following RISC-V memory layout)
    // These define the starting addresses for different memory segments
    uint32_t codeSegmentStart = 0x00000000; // Start of executable code
    uint32_t dataSegmentStart = 0x10000000; // Start of data segment
    uint32_t heapStart = 0x10008000;        // Heap memory start
    uint32_t stackStart = 0x7FFFFFD0;       // Stack memory start

    // Stores the generated instructions with their addresses
    vector<pair<string, string>> instructions;

    // Will store data segment information (for future implementation)
    vector<pair<string, string>> dataSegment;

    // Converts register name (e.g., "x1") to register number
    // Throws an error for invalid register names
    uint32_t parseRegister(const string &reg)
    {
        if (reg[0] != 'x')
            throw runtime_error("Invalid register format");
        return stoi(reg.substr(1)); // Extract number after 'x'
    }

    // Parses immediate values, handling:
    // 1. Labeled addresses
    // 2. Hexadecimal numbers
    // 3. Decimal numbers
    int32_t parseImmediate(const string &imm, uint32_t currentAddress = 0)
    {
        // Check if the immediate is a previously defined label
        if (symbolTable.find(imm) != symbolTable.end())
        {
            return symbolTable[imm] - currentAddress;
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
    // Each function takes instruction components and generates 32-bit machine code

    // R-type instruction encoding (arithmetic and logic operations)
    // Example instructions: add, sub, and, or, etc.
    uint32_t encodeRType(const string &opcode, uint32_t rd, uint32_t rs1,
                         uint32_t rs2, uint32_t funct3, uint32_t funct7)
    {
        // Bit-level encoding of R-type instruction
        return (funct7 << 25)                // Func7 bits (top 7 bits)
               | (rs2 << 20)                 // Source register 2 (bits 20-24)
               | (rs1 << 15)                 // Source register 1 (bits 15-19)
               | (funct3 << 12)              // Func3 bits (bits 12-14)
               | (rd << 7)                   // Destination register (bits 7-11)
               | stoul(opcode, nullptr, 16); // Opcode (bottom 7 bits)
    }

    // TODO: Add remaining encoding functions (SB, U, UJ types)

    // FIRST PASS: Collect Symbol Table
    // Scans the entire assembly file to:
    // 1. Identify labels
    // 2. Calculate their memory addresses
    void firstPass(const string &filename)
    {
        ifstream file(filename);
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

                // Remove label from line
                line = line.substr(line.find(':') + 1);
                line = regex_replace(line, regex("^\\s+"), "");
            }

            // Increment address for text segment instructions
            if (currentSegment == ".text" && !line.empty())
            {
                currentAddress += 4; // Each instruction is 4 bytes
            }
        }
    }

    // SECOND PASS: Generate Machine Code
    // Translates assembly instructions to their binary representation

    // Main instruction processing method (TODO: Full implementation)
    string processInstruction(const string &line, uint32_t currentAddress)
    {
        // Placeholder for full instruction processing logic
        // Would parse different instruction types and generate machine code
        throw runtime_error("Instruction processing not fully implemented");
    }

public:
    // Main assembler method

    void assemble(const string &inputFile, const string &outputFile)
    {
        // Collect symbol information in first pass
        firstPass(inputFile);

        // Generate machine code in second pass
        secondPass(inputFile);

        // Write machine code to output file
        ofstream outFile(outputFile);

        for (const auto &pair : instructions)
        {
            outFile << pair.first << " " << pair.second << endl;
        }
    }
};

int main()
{

    RISCVAssembler assembler;
    assembler.assemble("input.asm", "output.mc");

    return 0;
}
