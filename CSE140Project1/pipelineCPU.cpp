/*Spring 2021 CSE 140 project: Edwardo Raygoza and Alex Taylor */
#include <iostream>
#include<stdio.h>
#include<fstream>
#include<string>
#include<bitset>
using namespace std;

// Object for reading from the file
//each file has its own stream buffer
ifstream fin;

// Signals to store in IF_ID pipeline stage
struct IF_ID{
	int pc_4;
	int unsigned instructionRegister;
};

// Signals to store in ID_IE pipeline stage
struct ID_IE{
	int pc_4;
	int	read_data1;
	int	read_data2;
	int	immediate;
	int	funct;
	int	rd;
	int	rt;
	int	alu_src_mux;
	int	mem_to_reg;
	int	reg_dst_mux;
	int	reg_write;
	int	mem_read;
	int	mem_write;
	int	branch;
	int	inst_type;
};

// Signals to store in IE_MEM pipeline stage
struct IE_MEM {
	int pc_add;
	int	alu_out;
	int	alu_zero;
	int	read_data2;
	int	reg_dst;
	int	mem_to_reg;
	int	reg_write;
	int	mem_read;
	int	mem_write;
	int	branch;
	int	nop;
};

// Signals to store in MEM_WB pipeline stage
struct MEM_WB {
	int alu_out;
	int	mem_data;
	int	reg_dst;
	int	mem_to_reg;
	int	reg_write;
	int	nop;
};

// Initialzing pipeline signals to zero
IF_ID if_id_reg = { 0 };
IF_ID temp_if_id = { 0 };
ID_IE id_ie_reg = { 0 };
ID_IE temp_id_ie = { 0 };
IE_MEM ie_mem_reg = { 0 };
MEM_WB mem_wb_reg = { 0 };

// Array of register file names to show which register is modified
string registerNames[32] = { 
"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
"$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

// Global variables for register file, data memory, pc, instruction and control signals
int check = 0, check1 = 0;
int registerFile[32];
int d_mem[32];
int mem_data;
int pc = 0, next_pc;
int unsigned instructionRegister;
int NOP = 0;
int branch_target, jump_target;
int alu_zero = 0, alu_out = 0;
int read_data1, read_data2;
int alu_op1, alu_op2;
int alu_src_mux = 0, mem_to_reg = 0, pc_src_mux = 0, reg_dst_mux = 0, reg_write = 0, mem_read = 0, mem_write = 0, branch = 0, inst_type = 0, jump = 0, ALU_op = 0;
int total_clock_cycles = 0;
int opcode, rd, rs, rt, shamt, funct, reg_dst;
int immediate;

// Function to update all pipeline signals
void pipelineRegisterUpdate(){
	int temp;
	if ((check1 == 1) || (ie_mem_reg.nop != 1 && mem_wb_reg.nop != 1 && NOP != 1)) {
		{
		if (ie_mem_reg.branch && ie_mem_reg.alu_zero) {
			temp = branch_target;
		} else {
			temp = next_pc;
		}
		if (jump) {
			pc = jump_target;
		} else {
			pc = temp;
		}
	}
	} else {
		pc = pc;
	}
	if ((ie_mem_reg.nop != 1 && mem_wb_reg.nop != 1 && NOP != 1) || check1 == 1) {
		if (total_clock_cycles > 0) {
			cout << "pc is modified to 0x" << hex << pc << endl;
		}
	}
	check = check1;
	check1 = mem_wb_reg.nop;
	//mem_wb_reg****
	mem_wb_reg.alu_out = ie_mem_reg.alu_out;
	mem_wb_reg.mem_data = mem_data;
	mem_wb_reg.reg_dst = ie_mem_reg.reg_dst;
	mem_wb_reg.mem_to_reg = ie_mem_reg.mem_to_reg;
	mem_wb_reg.reg_write = ie_mem_reg.reg_write;

	if (ie_mem_reg.branch && ie_mem_reg.alu_zero) {
		cout << "control hazard detected (3 instruction flushes)" << endl;
        cout << endl;
		if_id_reg = { 0 };
		id_ie_reg = { 0 };
		ie_mem_reg = { 0 };
	} else {

		//ie_mem_reg
		ie_mem_reg.pc_add = branch_target;
		ie_mem_reg.alu_out = alu_out;
		ie_mem_reg.alu_zero = alu_zero;
		ie_mem_reg.read_data2 = id_ie_reg.read_data2;
		ie_mem_reg.reg_dst = reg_dst;
		ie_mem_reg.mem_to_reg = id_ie_reg.mem_to_reg;
		ie_mem_reg.reg_write = id_ie_reg.reg_write;
		ie_mem_reg.mem_read = id_ie_reg.mem_read;
		ie_mem_reg.mem_write = id_ie_reg.mem_write;
		ie_mem_reg.branch = id_ie_reg.branch;

		//id_ie_reg
		id_ie_reg.pc_4 = if_id_reg.pc_4;
		id_ie_reg.read_data1 = read_data1;
		id_ie_reg.read_data2 = read_data2;
		id_ie_reg.immediate = immediate;
		id_ie_reg.rd = rd;
		id_ie_reg.rt = rt;
		id_ie_reg.alu_src_mux = alu_src_mux;
		id_ie_reg.mem_to_reg = mem_to_reg;
		id_ie_reg.reg_dst_mux = reg_dst_mux;
		id_ie_reg.reg_write = reg_write;
		id_ie_reg.mem_read = mem_read;
		id_ie_reg.mem_write = mem_write;
		id_ie_reg.branch = branch;

		id_ie_reg.inst_type = inst_type;
		id_ie_reg.funct = funct;
		
		if_id_reg.pc_4 = next_pc;
		if_id_reg.instructionRegister = instructionRegister;
		if (NOP == 1) {
			temp_if_id = if_id_reg;
			temp_id_ie = id_ie_reg;
			if_id_reg = { 0 };
			id_ie_reg = { 0 };
			ie_mem_reg.branch = id_ie_reg.branch;
		} else if (ie_mem_reg.nop == 1) {
			if_id_reg = { 0 };
			id_ie_reg = { 0 };
			ie_mem_reg.branch = id_ie_reg.branch;	
		} else if (mem_wb_reg.nop == 1) {
			if_id_reg = temp_if_id;
			id_ie_reg = temp_id_ie;
		}
		if (NOP == 1) {
			cout << "Data hazard detected" << endl;
		}
        cout << endl;
		mem_wb_reg.nop = ie_mem_reg.nop;
		ie_mem_reg.nop = NOP;
	}
}

// Function to write back the value in the register file
void writeback() {
	int write_data;
	if (mem_wb_reg.mem_to_reg) {
		write_data = mem_wb_reg.mem_data;
	}
	else{
		write_data = mem_wb_reg.alu_out;
	}
	if (mem_wb_reg.reg_write) {
		registerFile[mem_wb_reg.reg_dst] = write_data;
	}

}

//Function to access the data memory
//mem_stage
void mem() {
	if (ie_mem_reg.mem_read) {
		mem_data = d_mem[ie_mem_reg.alu_out / 4];
	} else if (ie_mem_reg.mem_write) {
		d_mem[ie_mem_reg.alu_out / 4] = ie_mem_reg.read_data2;
	}
}

//Execution stage 
//Function to control the operations of ALU 
//generates proper alu op based on instruction type either being 0,1,2
void ALU_Control() {
	switch (id_ie_reg.inst_type) {
	case 0:
		ALU_op = 2;
		break;
	case 1:
		ALU_op = 6;
		break;
	case 2:
		switch (id_ie_reg.funct) {
		case 32:
			ALU_op = 2;
			break;
		case 34:
			ALU_op = 6;
			break;
		case 36:
			ALU_op = 0;
			break;
		case 37:
			ALU_op = 1;
			break;
		case 42:
			ALU_op = 7;
			break;
		case 39:
			ALU_op = 12;
			break;
		}
		break;
	}
}

void execution() {
	ALU_Control();

	alu_op1 = id_ie_reg.read_data1;
	
    if (alu_src_mux) {
        alu_op2 = immediate;
    } else{
        alu_op2 = registerFile[rt];
    }

	switch (ALU_op) {
	case 2:
		alu_out = alu_op1 + alu_op2;
		break;
	case 6:
		alu_out = alu_op1 - alu_op2;
		break;
	case 0:
		alu_out = alu_op1 & alu_op2;
		break;
	case 1:
		alu_out = alu_op1 | alu_op2;
		break;
	case 7:
		if (alu_op1 < alu_op2) {
			alu_out = 1;
		} else {
			alu_out = 0;
		}
		break;
	case 12:
		alu_out = ~(alu_op1 | alu_op2);
		break;
	}
	if (alu_out == 0) {
		alu_zero = 1;
	} else {
		alu_zero = 0;
	}
	branch_target = id_ie_reg.pc_4 + id_ie_reg.immediate;
	reg_dst = id_ie_reg.reg_dst_mux ? id_ie_reg.rd : id_ie_reg.rt;
	if ((reg_dst == rs || reg_dst == rt) && id_ie_reg.reg_write) {
		NOP = 1;
	} else {
		NOP = 0;
	}
}



// Function to generate the control signals for instructions
void controlUnit() {
	if (if_id_reg.instructionRegister != 0) {
		switch (opcode) {
		case 0:
			alu_src_mux = 0;
			mem_to_reg = 0;
			pc_src_mux = 0;
			reg_dst_mux = 1;
			reg_write = 1;
			mem_read = 0;
			mem_write = 0;
			branch = 0;
			inst_type = 2;
			jump = 0;
			break;
        //lw
		case 35:
			alu_src_mux = 1;
			mem_to_reg = 1;
			pc_src_mux = 0;
			reg_dst_mux = 0;
			reg_write = 1;
			mem_read = 1;
			mem_write = 0;
			branch = 0;
			inst_type = 0;
			jump = 0;
			break;
        //sw
		case 43:
			alu_src_mux = 1;
			mem_to_reg = 0;
			pc_src_mux = 0;
			reg_dst_mux = 0;
			reg_write = 0;
			mem_read = 0;
			mem_write = 1;
			branch = 0;
			inst_type = 0;
			jump = 0;
			break;
        //beq
		case 4:
			alu_src_mux = 0;
			mem_to_reg = 0;
			pc_src_mux = 0;
			reg_dst_mux = 0;
			reg_write = 0;
			mem_read = 0;
			mem_write = 0;
			branch = 1;
			inst_type = 1;
			jump = 0;
			break;
        //j
		case 2:
			alu_src_mux = 0;
			mem_to_reg = 0;
			pc_src_mux = 0;
			reg_dst_mux = 0;
			reg_write = 0;
			mem_read = 0;
			mem_write = 0;
			branch = 0;
			inst_type = 0;
			jump = 1;
			break;
		}
	} else {
		alu_src_mux = 0;
		mem_to_reg = 0;
		pc_src_mux = 0;
		reg_dst_mux = 0;
		reg_write = 0;
		mem_read = 0;
		mem_write = 0;
		branch = 0;
		inst_type = 0;
		jump = 0;
	}
}

//Decode stage
void decode() {
    //right bitshifting operator takes value of instructionRegister and shifts it right 26 times http://www.cplusplus.com/forum/beginner/95670/
	//right shifts variable     
	opcode = if_id_reg.instructionRegister >> 26 & 0x3f;
	switch (opcode) {
    //opcode is 0, R type instruction
	case 0:
		rs = (if_id_reg.instructionRegister >> 21) & 0x1F;
		rt = (if_id_reg.instructionRegister >> 16) & 0x1F;
		rd = (if_id_reg.instructionRegister >> 11) & 0x1F;

		shamt = (if_id_reg.instructionRegister >> 21) & 0x1F;
		funct = if_id_reg.instructionRegister & 0x3F;

		break;
	case 35:
    //opcode is 43, I type
	case 43:
		rs = (if_id_reg.instructionRegister >> 21) & 0x1F;
		rt = (if_id_reg.instructionRegister >> 16) & 0x1F;
		immediate = if_id_reg.instructionRegister & 0xffff;
		immediate = (immediate << 16) >> 16;
		break;
    //opcode is 4, I type
	case 4:
		rs = (if_id_reg.instructionRegister >> 21) & 0x1F;
		rt = (if_id_reg.instructionRegister >> 16) & 0x1F;
		immediate = if_id_reg.instructionRegister & 0xffff;
		immediate = (immediate << 16) >> 14;
		break;
	case 2:
		jump_target = (if_id_reg.instructionRegister & 0x3ffffff) << 2;
		jump_target = (next_pc & 0xf0000000) | jump_target;
		break;
	}
	read_data1 = registerFile[rs];
	read_data2 = registerFile[rt];
	controlUnit();
}

//fetch stage
int fetch() {
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
	while(i < 32) {
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

		total_clock_cycles += 1;

		// Printing the modified locations of register file and data memory with value and pc
		cout << "total_clock_cycles: " << dec << total_clock_cycles << endl;
	    if (mem_wb_reg.reg_write) {
		    cout << registerNames[mem_wb_reg.reg_dst] << " is modified to 0x" << hex << registerFile[mem_wb_reg.reg_dst] << endl;
	    } else if (ie_mem_reg.mem_write) {
            cout << "memory 0x" << hex << alu_out << " is modified to 0x" << hex << ie_mem_reg.read_data2 << endl;
	    }
	    if (ie_mem_reg.nop == 1) {
		    cout << "Data Hazard (NOP1)" << endl;
        } else if (mem_wb_reg.nop) {
		    cout << "Data Hazard (NOP2)" << endl;
	    }
		// Updating the pipeline signals
		pipelineRegisterUpdate();
	}
    for(int i = 0; i < 4; i++){
		decode();
		execution();
		mem();
		writeback();
		total_clock_cycles += 1;
		
        //prints
        cout << "total_clock_cycles: " << dec << total_clock_cycles << endl;
	    if (mem_wb_reg.reg_write) {
		    cout << registerNames[mem_wb_reg.reg_dst] << " is modified to 0x" << hex << registerFile[mem_wb_reg.reg_dst] << endl;
	    } else if (ie_mem_reg.mem_write) {
		    cout << "memory 0x" << hex << alu_out << " is modified to 0x" << hex << ie_mem_reg.read_data2 << endl;
	    } if (ie_mem_reg.nop == 1) {
		    cout << "Data Hazard (NOP1)" << endl;
        } else if (mem_wb_reg.nop) {
		    cout << "Data Hazard (NOP2)" << endl;
	    }
        // Updating the pipeline signals
		pipelineRegisterUpdate();
	}

	cout << "total_clock_cycles " << dec << total_clock_cycles << endl;
    cout << endl;
    cout << "program terminated" << endl;
    
	fin.close();
	return 0;
}