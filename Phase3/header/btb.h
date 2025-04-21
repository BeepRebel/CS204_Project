#ifndef BTB_H
#define BTB_H

#include <unordered_map>

// Branch Target Buffer class for branch prediction
class BTB {
private:
    std::unordered_map<int, std::pair<bool, int>> table; // Maps PC to (isTaken, targetAddress)

public:
    bool find(int pc) const; // check if PC exists 
    void enter(bool type, int pc, int to_take_address); // add entry
    bool predict(int pc) const; // predict if the branch is taken
    int getTarget(int pc) const; // get target address
};

#endif
