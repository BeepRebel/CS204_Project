#include <bits/stdc++.h>
using namespace std;

struct IF_ID {
    uint32_t PC = 0;
    uint32_t instruction = 0;
    bool is_dummy = false;
};

// ID/EX pipeline register
struct ID_EX {
    uint32_t PC = 0;
    std::string instruction_word;
    std::string rs1, rs2, rd;
    std::string operand1, operand2;
    std::string register_data;
    int alu_control_signal = -1;
    bool write_back_signal = false;
    int is_mem[2] = {-1, -1};  // 0 for load/store, -1 for no memory op
    std::string offset;
    uint32_t memory_address = 0;
    bool is_dummy = false;
    std::string asm_code;

    // For data forwarding control
    bool decode_forwarding_op1 = false;
    bool decode_forwarding_op2 = false;
    int pc_offset = 0;
    int inc_select = 0;
    int pc_select = 0;
    int return_address = 0;
};

// EX/MEM pipeline register
struct EX_MEM {
    uint32_t PC = 0;
    std::string register_data;
    std::string rd;
    bool write_back_signal = false;
    int is_mem[2] = {-1, -1};
    uint32_t memory_address = 0;
    bool is_dummy = false;
    int alu_control_signal = -1;
    std::string asm_code;
};

// MEM/WB pipeline register
struct MEM_WB {
    std::string register_data;
    std::string rd;
    bool write_back_signal = false;
    bool is_dummy = false;
    int alu_control_signal = -1;
    std::string asm_code;
};

// Processor class for managing execution
class Processor {
    public:
        std::unordered_map<uint32_t, std::string> MEM;
        std::string R[32];
    
        uint32_t next_PC;
        int inc_select, pc_select, return_address, pc_offset;
    
        // Counts
        int count_total_inst, count_alu_inst, count_mem_inst, count_control_inst;
        int count_branch_mispredictions;
    
        bool pipelining_enabled, terminate, all_dummy;
    
        Processor(std::string file_name);
        void load_program_memory(std::string file_name);
        void write_word(std::string address, std::string instruction);
        void IAG();
        void fetch(IF_ID &ifid, BranchPredictor &btb);
        void decode(ID_EX &idex, BranchPredictor &btb);
        void execute(ID_EX &idex);
        void mem(ID_EX &idex);
        void write_back(ID_EX &idex);
    };

// Branch Predictor class for managing branch predictions
class BranchPredictor {
        public:
            std::unordered_map<uint32_t, int> PHT;
            std::unordered_map<uint32_t, uint32_t> BTB;
        
            bool find(uint32_t PC);
            bool predict(uint32_t PC);
            void enter(bool taken, uint32_t PC, uint32_t target);
            uint32_t getTarget(uint32_t PC);
};

// Utility functions for number conversion
int nint(std::string hex_str);
std::string nhex(int value);
std::string sign_extend(std::string data);