#ifndef UTILS_H
#define UTILS_H

#include <string>

std::string nhex(int num); // Convert integer to hexadecimal string
int nint(const std::string& s, int base, int bits = 32); // Convert string to integer with sign extension
std::string bin_to_hex(const std::string& binary);  // Convert binary string to hexadecimal string
std::string hex_to_bin(const std::string& hex);      // Convert hexadecimal string to binary string
std::string sign_extend(const std::string data, int bit_length); // Sign-extend a binary or hex string

#endif // UTILS_H
