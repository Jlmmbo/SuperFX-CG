//make addr-mode funcs return effective addr instead of value @ that addr

typedef struct {
    byte bank;
    uint16_t addr;
}address;

typedef struct{
    char N:1;
    char V:1;
    char M:1;
    char X:1;
    char D:1;
    char I:1;
    char Z:1;
    char E:1;//E:emulation or carry
}StatusReg;

typedef struct {
    two_bytes X, Y;//index registers
    two_bytes D;//direct register
    two_bytes C;//accumulator
    StatusReg P;//processor status register
    uint16_t PC;//program counter
    two_bytes S;//stack address register
    byte DBR;//data bank register
    byte PBR;//program bank register
    byte* mem[256];//mem bank pointers
}CPUState;

typedef struct {
    byte* raw;
    long int size:24;//long to ensure at least 24 bits reg. int may only be 16
}Rom;


address inc_addr(address addr){
    uint32_t final_addr = addr.bank * 65536 + addr.addr + 1;
    return (address){final_addr % 65536, final_addr / 65536};
}

/*stub.
Will write rom to proper memory banks/regions, 
and allocate non-rom used banks (e.g. ram, sram, i/o &c.)*/
char write_rom(CPUState* cpu,Rom rom){
    byte rom_bitmask;
    if(rom_bitmask == 0);
    for (byte bank = 0; bank<0xFF; bank++){
        ;
    }
    return 0;
}


Rom test_rom = {(byte[]){0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA},8};



/*start up cpu, initialize regs, set pc to ffffd,c*/
void init_cpu(CPUState* cpu){
    cpu->D = (two_bytes){0x00, 0x00};
    cpu->DBR = 0x00;
    cpu->PBR = 0x00;
    cpu->S.h = 0x01;
    cpu->X.h = 0x00;
    cpu->Y.h = 0x00;
    cpu->P.M = 1;
    cpu->P.X = 1;
    cpu->P.D = 0;
    cpu->P.I = 1;
    cpu->P.E = 1;//set to emulation mode
    cpu->PC = cpu->mem[0][0xFFFD] * 256 + cpu->mem[0][0xFFFC];
    char err = write_rom(cpu,test_rom);
    if (err){
        Bdisp_AllClr_VRAM();
        PrintXY(1,1,"  GET MORE SPACE!!", 0, 0);
    }
}



//increment the program counter
uint16_t inc_PC(CPUState* cpu){
    cpu->PC++;
    return cpu->PC+1;
}

/*Fetch memory from address in bank.
 If bank has not already been allocated, will call sys_realloc(bank)*/
byte mem_fetch(CPUState* cpu, address addr){
    if(cpu->mem[addr.bank]==NULL){
        sys_realloc(cpu->mem[addr.bank], 65536 * sizeof(byte));
    }
    return cpu->mem[addr.bank][addr.addr];
}

/*set memory at adress in bank.
 If bank has not already been allocated, will call sys_realloc(bank)*/
char mem_set(CPUState* cpu, byte value, address addr){
    if(cpu->mem[addr.bank]==NULL){
        sys_realloc(cpu->mem[addr.bank], 65536 * sizeof(byte));
    }
    cpu->mem[addr.bank][addr.addr] = value;
    return 0;
}


//absolute
address a(CPUState* cpu, two_bytes addr){
    return (address){cpu->DBR, addr.h * 256 + addr.l};
}

//absolute indexed x
address ax(CPUState* cpu, two_bytes addr){
    return (address){cpu->DBR, addr.h * 256 + addr.l + cpu->X.h * 256 + cpu->X.l};
}

//absolute indexed y
address ay(CPUState* cpu, two_bytes addr){
    return (address){cpu->DBR, addr.h * 256 + addr.l + cpu->X.h * 256 + cpu->X.l};
}

//absolute long indexed x
address alx(CPUState* cpu, address addr){
    uint32_t effective_addr = addr.bank * 65536 + addr.addr + cpu->Y.h * 256 + cpu->Y.l;
    return (address){effective_addr / 65536, effective_addr % 65536};
}

//absolute long
address al(CPUState* cpu, address addr){
    return addr;
}

//direct indirect indexed x
address dxi(CPUState* cpu, byte offset){
    uint32_t direct_address = (cpu->D.h * 256 + cpu->D.l + offset + cpu->X.h * 256 + cpu->X.l) % 65536;
    return (address){cpu->DBR, direct_address};
}

//direct indexed x
address dx(CPUState* cpu, byte offset){
    return (address){0x00, cpu->D.h * 256 + cpu->D.l + offset + cpu->X.h * 256 + cpu->X.l};
}

//direct indexed y
address dy(CPUState* cpu, byte offset){
    return (address){0x00, cpu->D.h * 256 + cpu->D.l + offset + cpu->Y.h * 256 + cpu->Y.l};
}

//direct indirect indexed y
address diy(CPUState* cpu, byte offset){
    uint32_t direct_address = cpu->D.h * 256 + cpu->D.l + offset + cpu->DBR * 65536 + cpu->Y.h * 256 + cpu->Y.l;
    return (address){direct_address / 65536, direct_address % 65536};
}

//direct long indirect indexed y
address dliy(CPUState* cpu, byte offset){
    uint32_t direct_addr = (cpu->D.h * 256 + cpu->D.l + offset + cpu->Y.h * 256 + cpu->Y.l) % 16777216;
    return (address){direct_addr / 65536, (direct_addr) % 65536};
}

//direct long indirect
address dli(CPUState* cpu, byte offset){
    return (address){0x00, cpu->D.h * 256 + cpu->D.l + offset};
}

//direct indirect
address di(CPUState* cpu, byte offset){
    return (address){cpu->DBR, cpu->D.h * 256 + cpu->D.l + offset};
}

//direct
address d(CPUState* cpu, two_bytes addr){
    return (address){0x00, addr.h * 256 + addr.l};
}

//relative long
address rl(CPUState* cpu){
    return (address){0x00,0x00};
}

//relative
address r(CPUState* cpu){
    return (address){0x00,0x00};
}

//stack
address s(CPUState* cpu){
    return (address){0x00,cpu->S.h * 256 + cpu->S.l}; //??????????
}

//stack relative
address ds(CPUState* cpu, byte offset){
    return (address){0x00, cpu->S.h * 256 + cpu->S.l + offset};
}

//stack relative indirect indexed y
address dsiy(CPUState* cpu, byte offset){
    address base_addr = {cpu->DBR, cpu->S.h * 256 + cpu->S.l + offset};
    return (address){0x00,};
}


two_bytes ORA(CPUState* cpu, address v){
    if (cpu->P.M){
        cpu->C.l |= mem_fetch(cpu, v);
        return cpu->C;
    }
    cpu->C = (two_bytes){cpu->C.h | mem_fetch(cpu, v), cpu->C.l | mem_fetch(cpu, inc_addr(v))};
    return cpu->C;
}

two_bytes AND(CPUState* cpu, address v){
    if (cpu->P.M){
        cpu->C.l &= mem_fetch(cpu, v);
        return cpu->C;
    }
    cpu->C = (two_bytes){cpu->C.h & mem_fetch(cpu, v), cpu->C.l & mem_fetch(cpu, inc_addr(v))};
    return cpu->C;
}

two_bytes EOR(CPUState* cpu, address v){
    if (cpu->P.M){
        cpu->C.l ^= mem_fetch(cpu, v);
        return cpu->C;
    }
    cpu->C = (two_bytes){cpu->C.h ^ mem_fetch(cpu, v), cpu->C.l ^ mem_fetch(cpu, inc_addr(v))};
    return cpu->C;

}

two_bytes ADC(CPUState* cpu, address v){
    if (cpu->P.M){
            two_bytes ans = add_2_1(cpu->C, mem_fetch(cpu, v));
            cpu->C.l = ans.l;
            if (ans.h != 0x00){
                cpu->P.E = 1;
            }
            return cpu->C;
        }
    cpu->C = add_2_2(cpu->C, (two_bytes){mem_fetch(cpu, v), mem_fetch(cpu, inc_addr(v))});
    return cpu->C;
}

two_bytes STA(CPUState* cpu, address v){
    if (cpu->P.M){
            cpu->mem[v.bank][v.addr] = cpu->C.l;
            return cpu->C;
        }
    cpu->mem[v.bank][v.addr] = cpu->C.l;
    address inc_ed = inc_addr(v);
    cpu->mem[inc_ed.bank][inc_ed.addr] = cpu->C.h;
    return cpu->C;
}

two_bytes LDA(CPUState* cpu, address v){
    if (cpu->P.M){
            cpu->C.l = mem_fetch(cpu, v);
            return cpu->C;
        }
    cpu->C.l = mem_fetch(cpu, v);
    address inc_ed = inc_addr(v);
    cpu->C.h = mem_fetch(cpu, inc_ed);
    return cpu->C;
}

two_bytes CMP(CPUState* cpu, address v){
    if (cpu->P.M){
        byte val = mem_fetch(cpu, v);
        byte ans = cpu->C.l - val;
        cpu->P.Z = ans == 0x00;
        cpu->P.V = ((cpu->C.l >> 7) && (val >> 7)) && (ans >> 7 == 0);
        cpu->P;
        return cpu->C;
    }
    return cpu->C;
}

two_bytes SBC(CPUState* cpu, address v){
    if (cpu->P.M){
            two_bytes ans = sub_2_1(cpu->C, mem_fetch(cpu, v));
            cpu->C.l = ans.l;
            if (ans.h != 0x00){
                cpu->P.E = 1;
            }
            return cpu->C;
        }
    cpu->C = sub_2_2(cpu->C, (two_bytes){mem_fetch(cpu, v), mem_fetch(cpu, inc_addr(v))});
    return cpu->C;
}

/*run instruction
returns 0 for success; 1: somehow illegal opcode; 2:...*/
char run_instr_cpu(CPUState* cpu){
    char instr = mem_fetch(cpu, (address){cpu->PBR, cpu->PC});
    inc_PC(cpu);
    char type = instr & 0b00000011;
    char addr_mode = instr & 0b00011100;
    char instr_type = instr & 0b11100000;
    if (type == 0b01){//most popular instructions should get checked first
        address effective_addr;
        two_bytes operand;
        operand.l = cpu->mem[cpu->PBR][cpu->PC];
        operand.h = cpu->mem[cpu->PBR][cpu->PC+1];
        switch (addr_mode){
            case 0b000://direct indirect indexed x
                effective_addr = dxi(cpu, operand.l);
                break;
            case 0b001://direct
                effective_addr = d(cpu, operand);
                break;
            case 0b010://immediate
                effective_addr = (address){cpu->PBR, cpu->PC};
                break;
            case 0b011://absolute
                effective_addr = a(cpu, operand);
                break;
            case 0b100://direct indirect indexed y
                effective_addr = diy(cpu, operand.l);
                break;
            case 0b101://direct indexed x
                effective_addr = dx(cpu, operand.l);
                break;
            case 0b110://absolute indexed y
                effective_addr = ay(cpu, operand);
                break;
            case 0b111://absolute indexed x
                effective_addr = ax(cpu, operand);
                break;
            default:
                return 1;
        }
        switch (instr_type){
            case 0b000:
                ORA(cpu, effective_addr);
                break;
            case 0b001:
                AND(cpu, effective_addr);
                break;
            case 0b010:
                EOR(cpu, effective_addr);
                break;
            case 0b011:
                ADC(cpu, effective_addr);
                break;
            case 0b100:
                STA(cpu, effective_addr);
                break;
            case 0b101:
                LDA(cpu, effective_addr);
                break;
            case 0b110:
                CMP(cpu, effective_addr);
                break;
            case 0b111:
                SBC(cpu, effective_addr);
                break;
            default:
                return 1;
        }
        if(cpu->P.M == 1){
            inc_PC(cpu);
        }
        else{
            inc_PC(cpu);
            inc_PC(cpu);
        }
    }
    else if (type == 0b00){
        if (addr_mode == 0b100){//branch instructions
            byte take_branch;
            if (instr_type == 0b000){//BPL
                take_branch = !(cpu->P.N);
            }
            else if (instr_type == 0b001){//BMI
                take_branch = cpu->P.N;
            }
            else if (instr_type == 0b010){//BVC
                take_branch = !(cpu->P.V);
            }
            else if (instr_type == 0b011){//BVS
                take_branch = cpu->P.V;
            }
            else if (instr_type == 0b100){//BCC
                take_branch = !(cpu->P.E);
            }
            else if (instr_type == 0b101){//BCS
                take_branch = cpu->P.E;
            }
            else if (instr_type == 0b110){//BNE
                take_branch = !(cpu->P.Z);
            }
            else if (instr_type == 0b111){//BEQ
                take_branch = cpu->P.Z;
            }
            inc_PC(cpu);
            if(take_branch){
                signed char operand = mem_fetch(cpu, (address){cpu->PBR, cpu->PC});
                cpu->PC += operand;
            }
        }
        else if(addr_mode == 0b110){//status flag stuff
            if (instr_type == 0b000){//CLC
                cpu->P.E = 0;
            }
            else if(instr_type == 0b001){//SEC
                cpu->P.E = 1;
            }
            else if(instr_type == 0b010){//CLI
                cpu->P.I = 0;
            }
            else if(instr_type == 0b011){//SEI
                cpu->P.I = 1;
            }
            else if(instr_type == 0b100){//TYA
                cpu->C = cpu->Y;
            }
            else if(instr_type == 0b101){//CLV
                cpu->P.V = 0;
            }
            else if(instr_type == 0b110){//CLD
                cpu->P.D = 0;
            }
            else if(instr_type == 0b111){//SED
                cpu->P.D = 1;
            }
        }
        else if (addr_mode == 0b011){//absolute
            address effective_addr;
            if(instr_type == 0b000){//TSB

            }
            else if (instr_type == 0b001){//BIT

            }
            else if (instr_type == 0b010){//JMP a
                
            }
            else if (instr_type == 0b011){//JMP ai
                
            }
            else if (instr_type == 0b100){//STY
                
            }
            else if (instr_type == 0b101){//LDY
                
            }
            else if (instr_type == 0b110){//CPY
                
            }
            else if (instr_type == 0b111){//CPX
                
            }
        }
    }
    else if (type == 0b10){

    }
    else if (type == 0b11){

    }
    return 0;
}

void cycle_cpu(CPUState* cpu){
    run_instr_cpu(cpu);
}