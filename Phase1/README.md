# RISC-V Assembler

## Overview
This project is a simple RISC-V assembler written in C++ that translates assembly code into machine code. It supports 31 instructions and processes input from `input.asm`, generating the corresponding machine code in `output.mc`.

## Features
- Supports 31 RISC-V 32-bit instructions.
- Reads assembly instructions from `input.asm`.
- Outputs machine code to `output.mc` with address, machine code, assembly instruction, and opcode breakdown.
- Supports assembler directives: `.text`, `.data`, `.byte`, `.half`, `.word`, `.dword`, `.asciiz`.

## Usage
### Running the Assembler
```sh
./riscv_assembler
```

### Input File Format (`input.asm`)
The input file should contain RISC-V assembly instructions, one per line, in standard syntax. Example:
```
add x1, x2, x3
andi x5, x6, 10
```

### Output File Format (`output.mc`)
The output file will contain the corresponding machine code with address, instruction, and opcode breakdown. Example:
```
0x0 0x003100B3 , add x1,x2,x3 # 0110011-000-0000000-00001-00010-00011-NULL
0x4 0x00A37293 , andi x5,x6,10 # 0010011-111-NULL-00101-00110-000000001010
```

## Installation
1. Clone the repository:
```sh
git clone https://github.com/yourusername/riscv-assembler.git
cd riscv-assembler
```
2. Compile the code:
```sh
g++ -o riscv_assembler assembler.cpp
```

## Authors
-Charvi Pahuja(2023csb1114)
-Krishna Agarwal(2023csb1131)
-Tamanna(2023csb1169)

## References
- [RISC-V Instruction Set Manual](https://riscv.org/technical/specifications/)

