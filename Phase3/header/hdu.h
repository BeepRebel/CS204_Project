#ifndef HDU_H
#define HDU_H

#include <tuple>
#include <vector>
#include <string>
#include <bitset>
#include <iostream>
#include "../header/state.h" //path

// Hazard Detection Unit class for managing data hazards
class HDU {
public:
    std::tuple<bool, int, std::pair<int, int>> data_hazard_stalling(std::vector<State*>& pipeline_instructions);  // detects data hazards and returns stall info

    // handle data hazard forwarding and return updated states
    std::tuple<int, bool, int, std::vector<State*>, std::pair<int, std::pair<int, std::vector<std::string>>>> 
    data_hazard_forwarding(std::vector<State*>& pipeline_instructions);
};

#endif
