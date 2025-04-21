#include "../header/btb.h"

bool BTB::find(int pc) const {
    return table.find(pc) != table.end();
}

// put new branch prediction entry into the BTB
void BTB::enter(bool type, int pc, int to_take_address) {
    // Update the BTB entry based on the branch type and target address
    if (type) {
        table[pc] = {true, to_take_address};
    } else if (to_take_address > pc) {
        table[pc] = {false, to_take_address};
    } else {
        table[pc] = {true, to_take_address};
    }
}

//chceck if branch at PC is taken 
bool BTB::predict(int pc) const {
    return table.at(pc).first;
}


int BTB::getTarget(int pc) const {
    return table.at(pc).second;
}
