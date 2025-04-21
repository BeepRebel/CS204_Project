#include "../header/utils.h"
#include <bits/stdc++.h>
using namespace std;

string nhex(int num)
{
    // this is to handle negative numbers
    if (num < 0)
    {
        num += (1LL << 32);
    }

    stringstream ss;
    ss << "0x" << setfill('0') << setw(8) << hex << num << dec;
    return ss.str();
}

// utility: to convert to int with sign extension.
int nint(const string &s, int base, int bits)
{
    // convert string to long long unsigned.
    unsigned long long num = stoull(s, nullptr, base);

    // this is to sign extend
    if (num >= (1ULL << (bits - 1)))
    {
        num -= (1ULL << bits);
    }

    return static_cast<int>(num);
}

string bin_to_hex(const string &binary)
{
    stringstream ss;
    ss << "0x" << hex << stoull(binary, nullptr, 2) << dec;
    return ss.str();
}

string hex_to_bin(const string &hex)
{
    string bin = "";
    for (char c : hex)
    {
        int value = (c >= '0' && c <= '9') ? (c - '0') : (10 + (c - 'a'));
        bin += std::bitset<4>(value).to_string();
    }
    return bin;
}

string sign_extend(std::string data, int bit_length)
{
    if (data.substr(0, 2) == "0x") // check if the input is in hexadecimal format
    {                                                             // handling hexadecimal input
        char highDigit = data[2];                                 // first digit after "0x"
        bool isNegative = (highDigit >= '8' && highDigit <= 'f'); // checking if the number is negative based on the most significant digit

        if (isNegative)
        {
            // if the number is negative, extend with 'f' to preserve sign
            data = "0x" + std::string(8 - (data.length() - 2), 'f') + data.substr(2); 
        }
        else
        {
            // if the number is positive, extend with '0' to preserve sign
            data = "0x" + std::string(8 - (data.length() - 2), '0') + data.substr(2); 
        }
    }
    else
    { // handling binary input
        if (data.length() < bit_length)
        {
            char sign_bit = data[0]; // get the sign bit ('1' for negative, '0' for positive)
            // extend the sign bit to match the required bit length
            data = std::string(bit_length - data.length(), sign_bit) + data;
        }
    }
    return data; // return the sign-extended value
}
