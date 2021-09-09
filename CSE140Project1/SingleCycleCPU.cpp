/*Spring 2021 CSE 140 project: Edwardo Raygoza and Alex Taylor */

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <bitset>
using namespace std;

// Array of register file names to show which register is modified
string registerNames[32] = { "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
"$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

//Global variables for register file, data memory, pc, instruction and control signals
int registerFile[32];
int d_mem[32];
int mem_data;
int pc = 0, next_pc;
int unsigned instructionRegister;
int branch_target = 0;
int jump_target = 0;
int alu_zero = 0,
alu_out = 0;
int alu_op1, alu_op2;
int alu_src_mux = 0,
memToReg = 0,
pc_src_mux = 0,
reg_dst_mux = 0,
RegWrite = 0,
mem_read = 0,
mem_write = 0,
branch = 0,
instructionType = 0,
jump = 0,
ALU_OP = 0;
int total_clock_cycles = 0;
int opcode, rd, rs, rt, shamt, funct; //variables for R instructions
int immediate;
// Object for reading from the file
//each file has its own stream buffer
ifstream fin;


//
void mem(){
	if (mem_read) {
		mem_data = d_mem[alu_out / 4];
    //if memory write is 1
	} else if (mem_write) {
		d_mem[alu_out / 4] = registerFile[rt];
	}
}

void execution(){
	alu_op1 = registerFile[rs];

    if (alu_src_mux) {
        alu_op2 = immediate;
    } else{
        alu_op2 = registerFile[rt];
    }
	switch (ALU_OP){
	//add operation
	case 2:
		alu_out = alu_op1 + alu_op2;
		break;
	//sub operation
	case 6:
		alu_out = alu_op1 - alu_op2;
		break;
	//and operation
	case 0:
		alu_out = alu_op1 & alu_op2;
		break;
	//or operation
	case 1:
		alu_out = alu_op1 | alu_op2;
		break;
	//slt operation
	case 7:
		if (alu_op1 < alu_op2){
			alu_out = 1;
		}
		else{
			alu_out = 0;
		}
		break;
	//nor
	case 12:
		alu_out = ~(alu_op1 | alu_op2);
		break;
	}
	if (alu_out == 0){
		alu_zero = 1;
	}
	else{
		alu_zero = 0;
	}
	//used to decide PC value of next cycle
	branch_target = next_pc + immediate;
}

//generates proper alu op based on instruction type either being 0,1,2
void ALU_Control() {
	switch (instructionType) {
	case 0:
		ALU_OP = 2;
		break;
	case 1:
		ALU_OP = 6;
		break;
	case 2:
		switch (funct) {
		case 32:
			ALU_OP = 2;
			break;
		case 34:
			ALU_OP = 6;
			break;
		case 36:
			ALU_OP = 0;
			break;
		case 37:
			ALU_OP = 1;
			break;
		case 42:
			ALU_OP = 7;
			break;
		case 39:
			ALU_OP = 12;
			break;
		}
		break;
	}
}

//generates control signals
void controlUnit(){
	switch (opcode){
	case 0:
		alu_src_mux = 0;
		memToReg = 0;
		pc_src_mux = 0;
		reg_dst_mux = 1;
		RegWrite = 1;
		mem_read = 0;
		mem_write = 0;
		branch = 0;
		instructionType = 2;
		jump = 0;
		break;
    //lw
	case 35:
		alu_src_mux = 1;
		memToReg = 1;
		pc_src_mux = 0;
		reg_dst_mux = 0;
		RegWrite = 1;
		mem_read = 1;
		mem_write = 0;
		branch = 0;
		instructionType = 0;
		jump = 0;
		break;
    //sw
	case 43:
		alu_src_mux = 1;
		memToReg = 0;
		pc_src_mux = 0;
		reg_dst_mux = 0;
		RegWrite = 0;
		mem_read = 0;
		mem_write = 1;
		branch = 0;
		instructionType = 0;
		jump = 0;
		break;
    //beq
	case 4:
		alu_src_mux = 0;
		memToReg = 0;
		pc_src_mux = 0;
		reg_dst_mux = 0;
		RegWrite = 0;
		mem_read = 0;
		mem_write = 0;
		branch = 1;
		instructionType = 1;
		jump = 0;
		break;
   
	case 2:
		alu_src_mux = 0;
		memToReg = 0;
		pc_src_mux = 0;
		reg_dst_mux = 0;
		RegWrite = 0;
		mem_read = 0;
		mem_write = 0;
		branch = 0;
		instructionType = 0;
		jump = 1;
		break;
	}
    //fuction call to assign alu op that will be used by execute by instruction type
	ALU_Control();
}

void decode(){
	//right bitshifting operator takes value of instructionRegister and shifts it right 26 times http://www.cplusplus.com/forum/beginner/95670/
	//right shifts variable     
	opcode = instructionRegister >> 26;
	switch (opcode){
    //opcode is 0, R type instruction
	case 0:
		rs = (instructionRegister >> 21) & 0x1F;
		rt = (instructionRegister >> 16) & 0x1F;
		rd = (instructionRegister >> 11) & 0x1F;

		shamt = (instructionRegister >> 21) & 0x1F;
		funct = instructionRegister & 0x3F;
		break;
	case 35:
        //opcode is 43, I type
	case 43:
		rs = (instructionRegister >> 21) & 0x1F;
		rt = (instructionRegister >> 16) & 0x1F;
		immediate = instructionRegister & 0xffff;
		immediate = (immediate << 16) >> 16;
		break;
        //opcode is 4, I type
	case 4:
		rs = (instructionRegister >> 21) & 0x1F;
		rt = (instructionRegister >> 16) & 0x1F;
		immediate = instructionRegister & 0xffff;
		immediate = (immediate << 16) >> 14;
		break;
        //opcode is 2, j type
	case 2:
		jump_target = (instructionRegister & 0x3ffffff) << 2;
		jump_target = (next_pc & 0xf0000000) | jump_target;
		break;
	}
	controlUnit();
}


void writeback(){
	int writeData;
	if (memToReg){
		writeData = mem_data;
	}
	else{
		writeData = alu_out;
	}

	if (RegWrite){
		if (reg_dst_mux){
			registerFile[rd] = writeData;
		}
		else{
			registerFile[rt] = writeData;
		}
	}
	//when insttruction finishes, increment cycles by 1 
	total_clock_cycles += 1;
}

int fetch(){
	//logic for copying 1 of 3 possible pc values(next_pc, branch_target, jump_target)
	int temp;
	if (branch && alu_zero){
		temp = branch_target;
	} else{
		temp = next_pc;
	}
	if (jump){
		pc = jump_target;
	} else{
		pc = temp;
	}
	if (total_clock_cycles>0){
		cout << "pc is modified to 0x" << hex << pc << endl;
	}
	cout << endl;
	//default constructor that initializes instructions with 32 bits of 0
	bitset<32> Instruction;
	int byte_decimal = (pc / 4) * 34;
	fin.seekg(byte_decimal); //Sets the position of the next character to be extracted from the input stream
	fin >> Instruction; // will instruction that has 32 values of 0's
	if (Instruction == 0){
		return 0;
	}
	//used to choose the next cycle pc value  among next_pc value, branch and jump addresses
	next_pc = pc + 4;
	//instruction gets converted to an unsigned long integer 10001110000010110000000000000100 -> 
	instructionRegister = (int)(Instruction.to_ulong());
	return 1;
}


int main() {
	
    //all register files and data memory are initialized to 0
	int i = 0;
	while(i < 32){
		registerFile[i];
		d_mem[i] = 0;
		i++;
	}
	//overrides register files with addresses at certain indexes
	registerFile[9] = 0x20;//register t1
	registerFile[10] = 0x5;//register t2
	registerFile[16] = 0x70;//register s0
	d_mem[28] = 0x5;//memory address 0x70
	d_mem[29] = 0x10;//memory address 0x74

    string programName;
	cout << "Enter the program file name to run: " << endl;
    cout << endl;
	cin >> programName;
	cout << endl;
	//opens files, fin is stream variable name, program name is name of file, iosa::in is stream operation mode
	fin.open(programName, ios::in);

	int in;
	// Iterating within the loop until the last instruction
	while (true) {
		// Fetching the instruction
		in = fetch();
		if (!in) {
			break;
		}
		// Decoding the instruction
		decode();
		// Executing the instruction
		execution();
		// Accessing the data memroy
		mem();
		// Updating the location of register file
		writeback();

		// Prints the modified locations of register file and data memory with value and pc
		cout << "total clock cycles: " << total_clock_cycles << endl;
		if (RegWrite){
			if (reg_dst_mux) {
				cout << registerNames[rd] << " is modified to 0x" << hex << registerFile[rd] << endl;
			} else {
				cout << registerNames[rt] << " is modified to 0x" << hex << registerFile[rt] << endl;
			}
		} else if (mem_write) {
			cout << "memory 0x" << hex << alu_out << " is modified to 0x" << hex << registerFile[rt] << endl;
		}
	}
	cout << "program terminated:" << endl;
	cout << "total execution time is " << total_clock_cycles << " cycles" << endl;

	fin.close();
	return 0;
}









