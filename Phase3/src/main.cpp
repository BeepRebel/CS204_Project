#include <bits/stdc++.h>
#include "../header/utils.h"
#include "../header/state.h"
#include "../header/btb.h"
#include "../header/hdu.h"
#include "../header/processor.h"
using namespace std;

struct Config {
    std::string prog_mc_file;
    bool pipelining_enabled;
    bool forwarding_enabled;
    bool print_registers_each_cycle;
    bool print_pipeline_registers;
    std::pair<bool, int> print_specific_pipeline_registers;
};

struct Statistics {
    int total_cycles = 0;
    int total_instructions = 0;
    float cpi = 0.0f;
    int mem_instructions = 0;
    int alu_instructions = 0;
    int control_instructions = 0;
    int stalls = 0;
    int data_hazards = 0;
    int control_hazards = 0;
    int branch_mispredictions = 0;
    int stalls_due_to_data_hazards = 0;
    int stalls_due_to_control_hazards = 0;
};

// Function to handle GUI display
void display(const std::vector<std::vector<std::string>>& pipeline_states, 
    const std::vector<int>& control_signals,
    const std::vector<std::vector<std::string>>& detailed_states) {
// GUI display implementation
}

// Function for pipelined execution
tuple<vector<State*>, bool, unsigned int> 
evaluate(Processor& processor, std::vector<State*>& pipeline_ins, BTB* btb) {
    processor.writeBack(pipeline_ins[0]);
    processor.mem(pipeline_ins[1]);
    processor.execute(pipeline_ins[2]);
    
    auto result = processor.decode(pipeline_ins[3], btb);
    bool control_hazard = std::get<0>(result);
    int control_pc = std::get<1>(result);
    bool entering = std::get<2>(result);
    int color = std::get<3>(result);
    
    processor.fetch(pipeline_ins[4], btb);
    
    std::vector<State*> new_pipeline;
    for (int i = 1; i < 5; i++) {
        new_pipeline.push_back(pipeline_ins[i]);
    }
    
    return {new_pipeline, control_hazard, control_pc};
}

void printStatistics(const Statistics& stats , ofstream &statfile) {
    statfile << "Total number of cycles: " << stats.total_cycles << std::endl;
    statfile << "Total instructions executed: " << stats.total_instructions << std::endl;
    statfile << "CPI: " << stats.cpi << std::endl;
    statfile << "Number of data-transfer (load/store) instructions executed: " << stats.mem_instructions << std::endl;
    statfile << "Number of ALU instructions executed: " << stats.alu_instructions << std::endl;
    statfile << "Number of Control instructions: " << stats.control_instructions << std::endl;
    statfile << "Number of stalls/bubbles in the pipeline: " << stats.stalls << std::endl;
    statfile << "Number of data hazards: " << stats.data_hazards << std::endl;
    statfile << "Number of control hazards: " << stats.control_hazards << std::endl;
    statfile << "Number of branch mispredictions: " << stats.branch_mispredictions << std::endl;
    statfile << "Number of stalls due to data hazards: " << stats.stalls_due_to_data_hazards << std::endl;
    statfile << "Number of stalls due to control hazards: " << stats.stalls_due_to_control_hazards << std::endl;
}

Config take_input() {
        Config config;
        // Default values
        config.prog_mc_file = "../test/test.mc";
        config.pipelining_enabled = true;
        config.forwarding_enabled = true;
        config.print_registers_each_cycle = false;
        config.print_pipeline_registers = true;
        config.print_specific_pipeline_registers = {false, 10};
    
        // Uncomment below to enable user input
        /*
        std::cout << "Enter MC file name: ";
        std::cin >> config.prog_mc_file;
    
        int choice;
        std::cout << "Enable pipelining? (1/0): ";
        std::cin >> choice;
        config.pipelining_enabled = (choice == 1);
    
        std::cout << "Enable forwarding? (1/0): ";
        std::cin >> choice;
        config.forwarding_enabled = (choice == 1);
    
        std::cout << "Print registers each cycle? (1/0): ";
        std::cin >> choice;
        config.print_registers_each_cycle = (choice == 1);
    
        std::cout << "Print pipeline registers? (1/0): ";
        std::cin >> choice;
        config.print_pipeline_registers = (choice == 1);
    
        std::cout << "Print specific pipeline registers? (1/0): ";
        std::cin >> choice;
        config.print_specific_pipeline_registers.first = (choice == 1);
        if (config.print_specific_pipeline_registers.first) {
            std::cout << "Enter instruction number: ";
            std::cin >> config.print_specific_pipeline_registers.second;
        }
        */
    
        return config;
}

int main() {
    Config config = take_input();
    Processor processor(config.prog_mc_file);
    HDU hdu;
    BTB btb;

    std::vector<int> s(12, 0);
    std::vector<std::vector<std::string>> l;
    std::vector<std::vector<std::string>> l_dash;
    std::vector<std::vector<int>> pc_tmp;
    std::vector<std::map<std::string, std::string>> data_hazard_pairs;
    std::vector<int> control_hazard_signals;
    std::map<int, std::string> stage = {{1, "fetch"}, {2, "decode"}, {3, "execute"}, {4, "memory"}, {5, "write_back"}};

    // Execution control signals
    unsigned int PC = 0;
    int clock_cycles = 0;
    bool prog_end = false;

    //storing counts 
    int number_of_stalls_due_to_control_hazards = 0;
    int number_of_data_hazards = 0;
    int number_of_stalls_due_to_data_hazards = 0;
    int total_number_of_stalls = 0;

    if (!config.pipelining_enabled) {
    // Multi-cycle implementation
    processor.pipelining_enabled = false;

    while (true) {
        State* instruction = new State(PC);

        processor.fetch(instruction, &btb);
        std::cout << "Fetched instruction: " << instruction->instruction_word << std::endl;

        std::vector<int> tmp_pc = {-1, -1, -1, -1,int(instruction->PC)};
        pc_tmp.push_back(tmp_pc);
        std::map<std::string, std::string> tmp_pair = {{"who", "-1"}, {"from_whom", "-1"}};
        data_hazard_pairs.push_back(tmp_pair);
        processor.decode(instruction, &btb);
        pc_tmp.push_back({-1, -1, -1, int(instruction->PC), -1});
        data_hazard_pairs.push_back({{"who", "-1"}, {"from_whom", "-1"}});

        
        if (processor.terminate) {
            prog_end = true;
            break;
        }
        processor.execute(instruction);
        pc_tmp.push_back({-1, -1, (int)instruction->PC, -1, -1});
        data_hazard_pairs.push_back({{"who", "-1"}, {"from_whom", "-1"}});


        processor.mem(instruction);
        pc_tmp.push_back({-1, (int)instruction->PC, -1, -1, -1});
        data_hazard_pairs.push_back({{"who", "-1"}, {"from_whom", "-1"}});


        processor.writeBack(instruction);
        pc_tmp.push_back({(int)instruction->PC, -1, -1, -1, -1});
        data_hazard_pairs.push_back({{"who", "-1"}, {"from_whom", "-1"}});
        control_hazard_signals.insert(control_hazard_signals.end(), 5, 0);
        clock_cycles++;
        
        if (config.print_registers_each_cycle) {
            std::cout << "CLOCK CYCLE: " << clock_cycles << std::endl;
            std::cout << "Register Data:-" << std::endl;
            for (int i = 0; i < 32; i++) {
                std::cout << "R" << i << ": " << processor.R[i] << " ";
            }
            std::cout << "\n" << std::endl;
        }

        PC = processor.next_PC;
        delete instruction;  // Clean up
    }    } else {
        // Pipelined implementation
        processor.pipelining_enabled = true;
        std::vector<State*> pipeline_instructions;
        
        for (int i = 0; i < 5; i++) {
            pipeline_instructions.push_back(new State(0));
            if (i < 4) pipeline_instructions[i]->is_dummy= true;
        }

        while (!prog_end) {
            if (!config.forwarding_enabled) {
                auto result = hdu.data_hazard_stalling(pipeline_instructions);
                bool hazard = get<0>(result);
                int hazard_count = get<1>(result);
                pair<int,int> hazard_pair = get<2>(result);

                vector<State*> old_states = pipeline_instructions;
                auto result2 = evaluate(processor, pipeline_instructions, &btb);
                pipeline_instructions = get<0>(result2);
                bool control_hazard = get<1>(result2);
                unsigned int control_pc = get<2>(result2);

                vector<int> tmp;
                for (int i = 0; i < 5; i++) {
                    if (old_states[i]->is_dummy) {
                        tmp.push_back(-1);  // "bubble"
                    } else {
                        tmp.push_back(old_states[i]->PC);
                    }
                }
                
                pc_tmp.push_back(tmp);
                data_hazard_pairs.push_back({{"who", to_string(hazard_pair.first)}, {"from_whom", to_string(hazard_pair.second)}});

                bool branch_taken = pipeline_instructions[3]->branch_taken;
                unsigned int branch_pc = pipeline_instructions[3]->next_pc;

                PC += 4;

                if (branch_taken && !hazard) {
                    PC = branch_pc;
                }

                if (control_hazard && !hazard) {
                    number_of_stalls_due_to_control_hazards++;
                    PC = control_pc;
                    pipeline_instructions.push_back(new State(PC));
                    pipeline_instructions[pipeline_instructions.size()-2]->is_dummy = true;
                }

                if (hazard) {
                    number_of_data_hazards += hazard_count;
                    number_of_stalls_due_to_data_hazards++;
                    
                    // Create new pipeline state
                    std::vector<State*> new_pipe;
                    new_pipe.insert(new_pipe.end(), pipeline_instructions.begin(), pipeline_instructions.begin() + 2);
                    
                    State* stall_state = new State(0);
                    stall_state->is_dummy = true;
                    new_pipe.push_back(stall_state);
                    
                    new_pipe.insert(new_pipe.end(), old_states.begin() + 3, old_states.end());
                    
                    // Clean up unused states and update pipeline
                    for (size_t i = 0; i < pipeline_instructions.size(); i++) {
                        if (std::find(new_pipe.begin(), new_pipe.end(), pipeline_instructions[i]) == new_pipe.end()) {
                            delete pipeline_instructions[i];
                        }
                    }
                    pipeline_instructions = new_pipe;
                    
                    PC -= 4;
                }

                if (!control_hazard && !hazard) {
                    pipeline_instructions.push_back(new State(PC));
                }

                pipeline_instructions[pipeline_instructions.size()-2]->next_pc = PC;

                prog_end = true;
                for (int i = 0; i < 4; i++) {
                    if (!pipeline_instructions[i]->is_dummy) {
                        prog_end = false;
                        break;
                    }
                }
                if (clock_cycles > 10000) {
                    std::cerr << "Aborting: too many cycles" << std::endl;
                    break;
                }
            }
            else {
                // Implementation with forwarding

                auto result = hdu.data_hazard_forwarding(pipeline_instructions);
                int data_hazard = std::get<0>(result);
                bool if_stall = std::get<1>(result);
                int stall_position = std::get<2>(result);
                pipeline_instructions = get<3>(result);
                pair<int, pair<int, vector<string>>> gui_pair = get<4>(result);

                vector<State*> old_states = pipeline_instructions;
                auto result2 = evaluate(processor, pipeline_instructions, &btb);
                pipeline_instructions = get<0>(result2);
                bool control_hazard = get<1>(result2);
                unsigned int control_pc = get<2>(result2);

                std::vector<int> tmp;
                for (int i = 0; i < 5; i++) {
                    if (old_states[i]->is_dummy) {
                        tmp.push_back(-1);  // "bubble"
                    } else {
                        tmp.push_back(old_states[i]->PC);
                    }
                }
                pc_tmp.push_back(tmp);
                data_hazard_pairs.push_back({{"who", "-1"}, {"from_whom", "-1"}});  // Simplified

                bool branch_taken = pipeline_instructions[3]->branch_taken;
                unsigned int branch_pc = pipeline_instructions[3]->next_pc;

                PC += 4;

                if (branch_taken && !if_stall) {
                    PC = branch_pc;
                }

                if (control_hazard && !if_stall) {
                    number_of_stalls_due_to_control_hazards++;
                    PC = control_pc;
                    pipeline_instructions.push_back(new State(PC));
                    pipeline_instructions[3]->is_dummy= true;
                }

                if (if_stall) {
                    number_of_stalls_due_to_data_hazards++;

                    if (stall_position == 0) {
                        // Handle stall at position 0
                        std::vector<State*> new_pipe;
                        new_pipe.push_back(pipeline_instructions[0]);
                        
                        State* stall_state = new State(0);
                        stall_state->is_dummy = true;
                        new_pipe.push_back(stall_state);
                        
                        new_pipe.insert(new_pipe.end(), old_states.begin() + 2, old_states.end());
                        
                        // Clean up and update
                        for (size_t i = 0; i < pipeline_instructions.size(); i++) {
                            if (std::find(new_pipe.begin(), new_pipe.end(), pipeline_instructions[i]) == new_pipe.end()) {
                                delete pipeline_instructions[i];
                            }
                        }
                        pipeline_instructions = new_pipe;
                        
                        PC -= 4;
                    } else if (stall_position == 1) {
                        // Handle stall at position 1
                        std::vector<State*> new_pipe;
                        new_pipe.insert(new_pipe.end(), pipeline_instructions.begin(), pipeline_instructions.begin() + 2);
                        
                        State* stall_state = new State(0);
                        stall_state->is_dummy = true;
                        new_pipe.push_back(stall_state);
                        
                        new_pipe.insert(new_pipe.end(), old_states.begin() + 3, old_states.end());
                        
                        // Clean up and update
                        for (size_t i = 0; i < pipeline_instructions.size(); i++) {
                            if (std::find(new_pipe.begin(), new_pipe.end(), pipeline_instructions[i]) == new_pipe.end()) {
                                delete pipeline_instructions[i];
                            }
                        }
                        pipeline_instructions = new_pipe;
                        
                        PC -= 4;
                    }
                }

                number_of_data_hazards += data_hazard;

                if (!control_hazard && !if_stall) {
                    pipeline_instructions.push_back(new State(PC));
                }

                pipeline_instructions[3]->next_pc = PC;

                // Reset forwarding flags
                for (auto& inst : pipeline_instructions) {
                    inst->decode_forwarding_op1 = false;
                    inst->decode_forwarding_op2 = false;
                }

                prog_end = true;
                for (int i = 0; i < 4; i++) {
                    if (!pipeline_instructions[i]->is_dummy) {
                        prog_end = false;
                        break;
                    }
                }
                
            }

            // Print specific pipeline register
            clock_cycles++;
            if(clock_cycles > 100) cout<< "Too many cycles";
            if (config.print_registers_each_cycle) {
                std::cout << "CLOCK CYCLE: " << clock_cycles << std::endl;
                std::cout << "Register Data:-" << std::endl;
                for (int i = 0; i < 32; i++) {
                    std::cout << "R" << i << ": " << processor.R[i] << " ";
                }
                std::cout << "\n" << std::endl;
            }
            if (config.print_specific_pipeline_registers.first) {
                for (auto& inst : pipeline_instructions) {
                    if (inst->PC / 4 == config.print_specific_pipeline_registers.second) {
                        if (!config.print_registers_each_cycle) {
                            std::cout << "CLOCK CYCLE: " << clock_cycles << std::endl;
                        }
                        std::cout << "Pipeline Registers:-" << std::endl;
                        std::cout << "Fetch # Decode => Instruction: " << pipeline_instructions[3]->instruction_word << std::endl;
                        std::cout << "Decode # Execute => Operand1: " << pipeline_instructions[2]->operand1
                                 << ", Operand2: " << pipeline_instructions[2]->operand2 << std::endl;
                        std::cout << "Execute # Memory => Data: " << pipeline_instructions[1]->register_data << std::endl;
                        std::cout << "Memory # WriteBack => Data: " << pipeline_instructions[0]->register_data << std::endl;
                        std::cout << "\n" << std::endl;
                    }
                }
            }
            // Print pipeline registers
            else if (config.print_pipeline_registers) {
                if (!config.print_registers_each_cycle) {
                    std::cout << "CLOCK CYCLE: " << clock_cycles << std::endl;
                }
                std::cout << "Pipeline Registers:-" << std::endl;
                std::cout << "Fetch # Decode => Instruction: " << pipeline_instructions[3]->instruction_word << std::endl;
                std::cout << "Decode # Execute => Operand1: " << pipeline_instructions[2]->operand1
                         << ", Operand2: " << pipeline_instructions[2]->operand2 << std::endl;
                std::cout << "Execute # Memory => Data: " << pipeline_instructions[1]->register_data << std::endl;
                std::cout << "Memory # WriteBack => Data: " << pipeline_instructions[0]->register_data << std::endl;
                std::cout << "\n" << std::endl;
            }
        }

        // Clean up pipeline states
        for (auto state : pipeline_instructions) {
            delete state;
        }
    }

    // Collect statistics
    Statistics stats;
    stats.total_cycles = clock_cycles;
    stats.total_instructions = processor.count_total_inst;
    stats.cpi = (stats.total_instructions > 0) ? float(stats.total_cycles) / stats.total_instructions : 0.0f;
    stats.mem_instructions = processor.count_mem_inst;
    stats.alu_instructions = processor.count_alu_inst;
    stats.control_instructions = processor.count_control_inst;
    stats.data_hazards = number_of_data_hazards;
    stats.control_hazards = stats.control_instructions; // as in your code
    stats.branch_mispredictions = processor.count_branch_mispredictions;
    stats.stalls_due_to_data_hazards = number_of_stalls_due_to_data_hazards;
    stats.stalls_due_to_control_hazards = number_of_stalls_due_to_control_hazards;
    stats.stalls = stats.stalls_due_to_data_hazards + stats.stalls_due_to_control_hazards;
    
    if (prog_end) {
        processor.write_data_memory();
        
        // Write statistics to file
        std::ofstream statfile("../output/stats.txt");
        printStatistics(stats, statfile);
        statfile.close();
        
        // Generate visualization data
        for (size_t i = 0; i < pc_tmp.size(); i++) {
            std::vector<std::string> tmp;
            for (int pc : pc_tmp[i]) {
                if (pc == -1) {
                    tmp.push_back("bubble");
                } else {
                    tmp.push_back(processor.get_code[pc]);
                }
            }
            l.push_back(tmp);
            
            std::vector<std::string> detailed_tmp;
            for (int j = 0; j < 5; j++) {
                if (config.forwarding_enabled && config.pipelining_enabled) {
                    // Handle forwarding visualization
                    detailed_tmp.push_back(processor.get_code[pc_tmp[i][j]]);
                } else {
                    detailed_tmp.push_back(processor.get_code[pc_tmp[i][j]]);
                }
            }
            l_dash.push_back(detailed_tmp);
        }
        
        // Resolve control + data hazard cases
        for (size_t i = 0; i < l.size(); i++) {
            if (data_hazard_pairs[i]["who"] == "3") {
                control_hazard_signals[i] = 0;
            }
        }
        
        // Display visualization
        // display(l, control_hazard_signals, l_dash);
    }

    return 0;
}
