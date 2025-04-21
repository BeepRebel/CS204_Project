#include "../header/hdu.h"

// detects data hazards and returns stall information
std::tuple<bool, int, std::pair<int, int>> HDU::data_hazard_stalling(std::vector<State*>& pipeline_instructions) {
    int count_data_hazard = 0; 
    bool data_hazard = false;   
    std::pair<int,int> gui_pair = {-1, -1}; 

    State* decode_state = pipeline_instructions[pipeline_instructions.size() - 2]; // Get decode state

    // Convert instruction word to binary string
    unsigned int hex_val = std::stoul(decode_state->instruction_word, nullptr, 16);
    std::bitset<32> bin_instruction(hex_val);
    std::string bin_str = bin_instruction.to_string();

    int decode_opcode = std::stoi(bin_str.substr(25, 7), nullptr, 2); // Extract opcode

    // Set source registers based on opcode
    if (decode_opcode == 19 || decode_opcode == 103 || decode_opcode == 3) {
        decode_state->rs1 = stoi(bin_str.substr(12, 5), nullptr, 2);
        decode_state->rs2 = -1; // No second source register
    } else if (!(decode_opcode == 23 || decode_opcode == 55 || decode_opcode == 111)) {
        decode_state->rs1 = stoi(bin_str.substr(12, 5), nullptr, 2);
        decode_state->rs2 = stoi(bin_str.substr(7, 5), nullptr, 2);
    }

    State* exe_state = pipeline_instructions[pipeline_instructions.size() - 3]; // Get execute state
    // Check for data hazards with the execute state
    if (exe_state->rd != -1 && exe_state->rd != 0 && !exe_state->is_dummy && !decode_state->is_dummy) {
        if (exe_state->rd == decode_state->rs1 || exe_state->rd == decode_state->rs2) {
            data_hazard = true; // Data hazard detected
            count_data_hazard++;
            gui_pair = {3, 2}; 
        }
    }

    State* mem_state = pipeline_instructions[pipeline_instructions.size() - 4]; // Get memory state
    // Check for data hazards with the memory state
    if (mem_state->rd != -1 && mem_state->rd != 0 && !mem_state->is_dummy && !decode_state->is_dummy) {
        if (mem_state->rd == decode_state->rs1 || mem_state->rd == decode_state->rs2) {
            data_hazard = true; // Data hazard detected
            count_data_hazard++;
            gui_pair = {3, 1}; 
        }
    }

    return {data_hazard, count_data_hazard, gui_pair}; // Return hazard info
}

// Handles data hazard forwarding and returns updated states
std::tuple<int, bool, int, std::vector<State*>, std::pair<int, std::pair<int, std::vector<std::string>>>> 
HDU::data_hazard_forwarding(std::vector<State*>& pipeline_instructions) {
    State* decode_state = pipeline_instructions[pipeline_instructions.size() - 2]; 
    State* exe_state = pipeline_instructions[pipeline_instructions.size() - 3]; 
    State* mem_state = pipeline_instructions[pipeline_instructions.size() - 4]; 
    State* wb_state = pipeline_instructions[pipeline_instructions.size() - 5]; 

    std::cout << "fetched instruction" << decode_state->instruction_word; // Log fetched instruction
    unsigned int hex_val = std::stoul(decode_state->instruction_word, nullptr, 16);
    std::bitset<32> bin_instruction(hex_val);
    std::string bin_str = bin_instruction.to_string();

    int decode_opcode = std::stoi(bin_str.substr(25, 7), nullptr, 2); // Extract opcode

    // Set source registers based on opcode
    if (decode_opcode == 19 || decode_opcode == 103 || decode_opcode == 3) {
        decode_state->rs1 = stoi(bin_str.substr(12, 5), nullptr, 2);
        decode_state->rs2 = -1; // No second source register
    } else if (!(decode_opcode == 23 || decode_opcode == 55 || decode_opcode == 111)) {
        decode_state->rs1 = stoi(bin_str.substr(12, 5), nullptr, 2);
        decode_state->rs2 = stoi(bin_str.substr(7, 5), nullptr, 2);
    }

    int data_hazard = 0; 
    bool if_stall = false; 
    int stall_position = 2; 
    std::pair<int, std::pair<int, std::vector<std::string>>> gui_pair = {-1, {-1, {}}}; 

    // Get opcodes for forwarding logic
    int exe_opcode = std::stoi(std::bitset<32>(std::stoul(exe_state->instruction_word, nullptr, 16)).to_string().substr(25, 7), nullptr, 2);
    int mem_opcode = std::stoi(std::bitset<32>(std::stoul(mem_state->instruction_word, nullptr, 16)).to_string().substr(25, 7), nullptr, 2);
    int wb_opcode = std::stoi(std::bitset<32>(std::stoul(wb_state->instruction_word, nullptr, 16)).to_string().substr(25, 7), nullptr, 2);

    // Forwarding logic for WB to MEM
    if (wb_opcode == 3 && mem_opcode == 35 && !wb_state->is_dummy && !mem_state->is_dummy) {
        if (wb_state->rd != -1 && wb_state->rd != 0 && wb_state->rd == mem_state->rs2) {
            mem_state->register_data = wb_state->register_data; // Forward data
            data_hazard++;
            gui_pair.second.second.push_back("forwarded from mem");
        }
    }

    // Forwarding logic for WB to EX
    if (wb_state->rd != -1 && wb_state->rd != 0 && !wb_state->is_dummy) {
        if (wb_state->rd == exe_state->rs1 && !exe_state->is_dummy) {
            exe_state->operand1 = wb_state->register_data; // Forward operand1
            data_hazard++;
            gui_pair.second.second.push_back("forwarded from mem"); 
        }

        if (wb_state->rd == exe_state->rs2 && !exe_state->is_dummy) {
            if (exe_opcode != 35) {
                exe_state->operand2 = wb_state->register_data; // Forward operand2
            } else {
                exe_state->register_data = wb_state->register_data; // Forward data for store
            }
            data_hazard++;
            gui_pair.second.second.push_back("forwarded from mem");
        }
    }

    // Forwarding logic for MEM to EX
    if (mem_state->rd != -1 && mem_state->rd != 0 && !mem_state->is_dummy) {
        if (mem_opcode == 3) { 
            if (exe_opcode == 35 && exe_state->rs1 == mem_state->rd && !exe_state->is_dummy) {
                data_hazard++; 
                if_stall = true; 
                stall_position = 0; 
                gui_pair = {2, {1, {}}}; 
            } else if ((exe_state->rs1 == mem_state->rd || exe_state->rs2 == mem_state->rd) && !exe_state->is_dummy) {
                data_hazard++; 
                if_stall = true; 
                stall_position = 0; 
                gui_pair = {2, {1, {}}}; 
            }
        } else {
            if (exe_state->rs1 == mem_state->rd && !exe_state->is_dummy) {
                exe_state->operand1 = mem_state->register_data; // Forward operand1
                data_hazard++;
                gui_pair.second.second.push_back("forwarded from execute"); 
            }

            if (exe_state->rs2 == mem_state->rd && !exe_state->is_dummy) {
                if (exe_opcode != 35) {
                    exe_state->operand2 = mem_state->register_data; // Forward operand2
                } else {
                    exe_state->register_data = mem_state->register_data; // Forward data for store
                }
                data_hazard++;
                gui_pair.second.second.push_back("forwarded from execute");
            }
        }
    }

    // Forwarding logic for control instructions
    if ((decode_opcode == 99 || decode_opcode == 103) && !decode_state->is_dummy) { // SB and jalr
        if (wb_state->rd != -1 && wb_state->rd != 0 && !wb_state->is_dummy) {
            if (wb_state->rd == decode_state->rs1) {
                decode_state->operand1 = wb_state->register_data; // Forward operand1
                decode_state->decode_forwarding_op1 = true; 
                data_hazard++;
                gui_pair.second.second.push_back("forwarded from mem"); 
            }
            if (wb_state->rd == decode_state->rs2) {
                decode_state->operand2 = wb_state->register_data; // Forward operand2
                decode_state->decode_forwarding_op2 = true; 
                data_hazard++;
                gui_pair.second.second.push_back("forwarded from mem"); 
            }
        }

        if (mem_state->rd != -1 && mem_state->rd != 0 && !mem_state->is_dummy) {
            if (mem_opcode == 3 && (mem_state->rd == decode_state->rs1 || mem_state->rd == decode_state->rs2)) {
                data_hazard++; 
                if_stall = true; 
                
                if (stall_position > 1) {
                    stall_position = 1; 
                    gui_pair = {3, {1, {}}}; 
                }
            } else {
                if (mem_state->rd == decode_state->rs1) {
                    decode_state->operand1 = mem_state->register_data; // Forward operand1
                    decode_state->decode_forwarding_op1 = true;
                    data_hazard++;
                    gui_pair.second.second.push_back("forwarded from execute"); 
                }
                if (mem_state->rd == decode_state->rs2) {
                    decode_state->operand2 = mem_state->register_data; // Forward operand2
                    decode_state->decode_forwarding_op2 = true;
                    data_hazard++;
                    gui_pair.second.second.push_back("forwarded from execute"); 
                }
            }
        }
    }

    // Check for data hazards with the execute state
    if (exe_state->rd != -1 && exe_state->rd != 0 && (exe_state->rd == decode_state->rs1 || exe_state->rd == decode_state->rs2) && !exe_state->is_dummy) {
        data_hazard++; 
        if_stall = true; 
        if (stall_position > 1) {
            stall_position = 1; 
            gui_pair = {3, {2, {}}}; 
        }
    }

    std::vector<State*> new_states = {wb_state, mem_state, exe_state, decode_state, pipeline_instructions.back()}; // Prepare new states
    return {data_hazard, if_stall, stall_position, new_states, gui_pair}; // Return hazard info
}