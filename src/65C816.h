#define CYCLE_TIME (62)
//~62 home ticks per 65c816 tick


typedef struct address{
    byte bank;
    uint16_t addr;
}address;

typedef struct StatusReg{
    char N:1;//bit 7: Negative
    char V:1;//bit 6: oVerflow
    char M:1;//bit 5: Memory/accumulator size (0: 16-bit, 1: 8-bit)
    char X:1;//bit 4: indeX size (0:16-bit, 1:8-bit)
    char D:1;//bit 3: Decimal mode
    char I:1;//bit 2: Interrupt disable (only on IRQ)
    char Z:1;//bit 1: Zero
    char E:1;//bit 0: Emulation or Carry
}StatusReg;

byte status_to_byte(StatusReg P){
    byte out = 0x00;
    out += P.E;
    out += P.Z << 1;
    out += P.I << 2;
    out += P.D << 3;
    out += P.X << 4;
    out += P.M << 5;
    out += P.V << 6;
    out += P.N << 7;
    return out;
}

StatusReg byte_to_status(byte P_byte){
    StatusReg P;
    P.E = P_byte & 0b00000001;
    P.Z = (P_byte & 0b00000010) >> 1;
    P.I = (P_byte & 0b00000100) >> 2;
    P.D = (P_byte & 0b00001000) >> 3;
    P.X = (P_byte & 0b00010000) >> 4;
    P.M = (P_byte & 0b00100000) >> 5;
    P.V = (P_byte & 0b01000000) >> 6;
    P.N = (P_byte & 0b10000000) >> 7;
    return P;
}

void status_apply_mask(StatusReg* P, byte set_bits, byte clear_bits){
    byte P_byte = status_to_byte(*P);
    P_byte = apply_mask(P_byte, set_bits, clear_bits);
    *P = byte_to_status(P_byte);
}

typedef struct CPUState{
    uint16_t X, Y;//index registers
    uint16_t D;//direct register
    two_bytes C;//accumulator
    StatusReg P;//processor status register
    uint16_t PC;//program counter
    two_bytes S;//stack address register
    byte DBR;//data bank register
    byte PBR;//program bank register
    byte* mem[256];//mem bank pointers
    int expected_time;//when the current instruction should finish
    int current_time;
}CPUState;

typedef struct Rom{
    byte* raw;
    long int size:24;//long to ensure at least 24 bits reg. int may only be 16
}Rom;

typedef struct instr_data{
    address (*addr_func)(CPUState*);//function to calculate effective address
    byte cycle_count;
    byte read_bytes;//num of bytes read, includes opcode
    void (*instr_func)(CPUState*, address);//function to excecute instruction
}instr_data;

void push(CPUState* cpu, byte val){
    if (cpu->P.M == 0) cpu->mem[0][cpu->S.h * 256 + cpu->S.l] = val;
    else cpu->mem[0][cpu->S.l] = val;
    cpu->S = sub_2_1(cpu->S, 0x01);//descending stack
}

byte pull(CPUState* cpu){
    cpu->S = add_2_1(cpu->S, 0x01);//??????????? if in emulation mode, does S.h decrement on page wrap?
    if (cpu->P.M == 0) return cpu->mem[0][cpu->S.h * 256 + cpu->S.l];
    return cpu->mem[0][cpu->S.l];
}

address inc_addr(address addr){
    uint32_t final_addr = addr.bank * 65536 + addr.addr + 1;
    return (address){final_addr % 65536, final_addr / 65536};
}

address add_addr(address addr, uint16_t amt){
    uint32_t final_addr = addr.bank * 65536 + addr.addr + amt;
    return (address){final_addr % 65536, final_addr / 65536};
}

/*
stub.
Will write rom to proper memory banks/regions, 
and allocate used non-rom banks (e.g. ram, sram, i/o &c.)*/
char write_rom(CPUState* cpu, Rom rom){
    byte rom_bitmask;
    if(rom_bitmask == 0);
    for (byte bank = 0; bank<0xFF; bank++){
        ;
    }
    return 0;
}


Rom test_rom = {(byte[]){0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA},8};//a bunch of NOPs



/*start up cpu, initialize regs, set load interrupt vector etc.*/
void init_cpu(CPUState* cpu, Rom rom){
    cpu->current_time = RTC_GetTicks();
    cpu->expected_time = RTC_GetTicks();
    cpu->D = 0x0000;
    cpu->DBR = 0x00;
    cpu->PBR = 0x00;
    cpu->S.h = 0x01;
    cpu->X &= 0x0011;
    cpu->Y &= 0x0011;
    cpu->P.M = 1;
    cpu->P.X = 1;
    cpu->P.D = 0;
    cpu->P.I = 1;
    cpu->P.E = 1;//set to emulation mode
    cpu->PC = cpu->mem[0][0xFFFD] * 256 + cpu->mem[0][0xFFFC];
    char err = write_rom(cpu, rom);
    if (err){//failed to allocate
        Bdisp_AllClr_VRAM();
        PrintXY(1,1,"  GET MORE SPACE!!", 0, 0);
    }
}



/*Fetch memory from address in bank.
 If bank has not already been allocated, calls sys_realloc(bank)*/
byte mem_fetch(CPUState* cpu, address addr){
    if(cpu->mem[addr.bank]==NULL){
        sys_realloc(cpu->mem[addr.bank], 65536 * sizeof(byte));
    }
    return cpu->mem[addr.bank][addr.addr];
}

/*set memory at address in bank.
 If bank has not already been allocated, will call sys_realloc(bank)*/
char mem_set(CPUState* cpu, byte value, address addr){
    if(cpu->mem[addr.bank]==NULL){
        sys_realloc(cpu->mem[addr.bank], 65536 * sizeof(byte));
    }
    cpu->mem[addr.bank][addr.addr] = value;
    return 0;
}


//absolute
address a(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){cpu->DBR, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256};
}

//absolute indexed indirect
address axi(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){0x00, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256 + cpu->X};
}

//absolute indexed X
address ax(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){cpu->DBR, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256 + cpu->X};
}

//absolute indexed Y
address ay(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){cpu->DBR, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256 + cpu->Y};
}

//absolute indirect
address ai(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){0x00, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256};
}

//absolute long indexed
address alx(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){mem_fetch(cpu, inc_addr(inc_addr(addr))), mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256 + cpu->X};
}

//absolute long
address al(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){mem_fetch(cpu, inc_addr(inc_addr(addr))), mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256};
}

/*address A(CPUState* cpu){
    return (address){cpu->PBR, cpu->PC};
}*/

//block move
address xyc(CPUState* cpu){
    return (address){cpu->DBR, mem_fetch(cpu, (address){cpu->PBR, cpu->PC})};
}

//direct indexed indirect
address dxi(CPUState* cpu){
    return (address){cpu->DBR, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC}) + cpu->X};
}

//direct indexed X
address dx(CPUState* cpu){
    return (address){0x00, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC}) + cpu->X};
}

//direct indexed Y
address dy(CPUState* cpu){
    return (address){0x00, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC}) + cpu->Y};
}

//direct indirect indexed
address diy(CPUState* cpu){
    return add_addr((address){cpu->DBR, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC})}, cpu->Y);
}

//direct long indirecte indexed
address dliy(CPUState* cpu){
    return add_addr((address){0x00, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC})}, cpu->Y);
}

//direct long indirect
address dli(CPUState* cpu){
    return (address){0x00, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC})};
}

//direct indirect
address di(CPUState* cpu){
    return (address){cpu->DBR, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC})};
}

//direct
address d(CPUState* cpu){
    return (address){0x00, cpu->D + mem_fetch(cpu, (address){cpu->PBR, cpu->PC})};
}

// immediate
address imm(CPUState* cpu){
    return (address){cpu->PBR, cpu->PC};
}

//no address
address nil(CPUState* cpu){
    return (address){0, 0};
}

//relative long
address rl(CPUState* cpu){
    return (address){cpu->PBR, cpu->PC};//////////////////////////////////////////////////////////////////////////////
}

//relative
address r(CPUState* cpu){
    return (address){cpu->PBR, cpu->PC};//////////////////////////////////////////////////////////////////////////////
}

//stack
address s(CPUState* cpu){
    return (address){0x00, cpu->S.h * 256 + cpu->S.l};
}

//direct stack
address ds(CPUState* cpu){
    return add_addr((address){0x00, cpu->S.h * 256 + cpu->S.l}, mem_fetch(cpu, (address){cpu->PBR, cpu->PC}));
}

//direct stack indirect indexed
address dsiy(CPUState* cpu){
    return add_addr((address){cpu->DBR, cpu->S.h * 256 + cpu->S.l + mem_fetch(cpu, (address){cpu->PBR, cpu->PC})}, cpu->Y);
}

//BReaK
void BRK(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        push(cpu, cpu->PBR);
        push(cpu, cpu->PC / 256);
        push(cpu, cpu->PC % 256 + 2);
        push(cpu, status_to_byte(cpu->P));
        cpu->PC = mem_fetch(cpu, (address){00, 0xffe6}) + mem_fetch(cpu, (address){00, 0xffe7}) * 256;
        cpu->P.I = 1;
        cpu->P.D = 0;
        return;
    }
    push(cpu, cpu->PC / 256);
    push(cpu, cpu->PC % 256 + 2);
    push(cpu, status_to_byte(cpu->P));
    cpu->PC = mem_fetch(cpu, (address){00, 0xfffe}) + mem_fetch(cpu, (address){00, 0xffff}) * 256;
    cpu->P.I = 1;
    cpu->P.D = 0;
    cpu->P.X = 1;
}

//OR with Accumulator
void ORA(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
        cpu->C = separate_bytes(v | bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(cpu, addr);
    cpu->C.l = cpu->C.l | v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//CO-Processor
void COP(CPUState* cpu, address addr){
     if(cpu->P.M == 0){//16-bit
        push(cpu, cpu->PBR);
        cpu->PBR = 0;
        push(cpu, cpu->PC / 256);
        push(cpu, cpu->PC % 256 + 2);
        push(cpu, status_to_byte(cpu->P));
        cpu->P.I = 1;
        cpu->P.D = 0;
     }
}

//Test and Set Bits
void TSB(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        mem_set(cpu, mem_fetch(cpu, addr) | cpu->C.l, addr);
        mem_set(cpu, mem_fetch(cpu, inc_addr(addr)) | cpu->C.h, inc_addr(addr));
        return;
    }
    mem_set(cpu, cpu->C.l | mem_fetch(cpu, addr), addr);
}

//Arithmetic Shift Left
void ASL(CPUState* cpu, address addr){
    if (cpu->P.M == 0){
        two_bytes val = (two_bytes){mem_fetch(cpu, addr), mem_fetch(cpu, inc_addr(addr))};
        cpu->P.E = val.h >> 7;//highest bit goes into carry
        val = add_2_2(val, val);//multiply by 2
        mem_set(cpu, val.h, addr);
        mem_set(cpu, val.l, inc_addr(addr));
        cpu->P.N = val.h >> 7; //highest bit is sign bit
        cpu->P.Z = (val.h == 0) && (val.l == 0);
        return;
    }
    byte val = mem_fetch(cpu, addr);
    cpu->P.E = val >> 7;
    val = val << 1;
    mem_set(cpu, val, addr);
    cpu->P.N = val >> 7;
    cpu->P.Z = (val == 0);
}

//PusH P
void PHP(CPUState* cpu, address addr){
    cpu->P.X = 1;//idk if break flag is set
    push(cpu, status_to_byte(cpu->P));
}

//Arithmetic Shift Left Accumulator
void ASLA(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        cpu->P.E = cpu->C.h >> 7;//highest bit goes into carry
        cpu->C.h = (cpu->C.h << 1) + (cpu->C.l >> 7);
        cpu->C.l = cpu->C.l << 1;
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    cpu->P.E = cpu->C.l >> 7;
    cpu->C.l = cpu->C.l << 1;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//PusH D
void PHD(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        push(cpu, cpu->D / 256);
        push(cpu, cpu->D % 256);
        return;
    }
    push(cpu, cpu->D % 256);
}

//Branch if PLus
void BPL(CPUState* cpu, address addr){
    char offset = mem_fetch(cpu, addr);
    if(cpu->P.N == 0){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//Test and Reset Bits
void TRB(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        two_bytes val = (two_bytes){mem_fetch(cpu, inc_addr(addr)), mem_fetch(cpu, addr)};
        mem_set(cpu, (val.l ^ cpu->C.l) & val.l, addr);//Set bits in C = clear bits in mem
        mem_set(cpu, (val.h ^ cpu->C.h) & val.h, inc_addr(addr));//somehow (val XOR C) & val does this
        return;
    }
    byte val = mem_fetch(cpu, addr);
    mem_set(cpu, (val ^ cpu->C.l) & val, addr);
}

//CLear Carry
void CLC(CPUState* cpu, address addr){
    cpu->P.E = 0;
}

//INCrement Accumulator
void INCA(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        cpu->C = add_2_1(cpu->C, 0x01);
        return;
    }
    cpu->C.l++;
}

//Transfer aCcumulator to Stack
void TCS(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        cpu->S = (two_bytes){cpu->C.h, cpu->C.l};
        return;
    }
    cpu->S.l = cpu->C.l;
}

//Jump to SubRoutine
void JSR(CPUState* cpu, address addr){
    push(cpu, cpu->PC / 256);
    push(cpu, cpu->PC % 256);
    cpu->PC = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr));
}

//AND with Accumulator
void AND(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
        cpu->C = separate_bytes(v & bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(cpu, addr);
    cpu->C.l = cpu->C.l & v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//Jump to Subroutine Long
void JSL(CPUState* cpu, address addr){
    push(cpu, cpu->PBR);
    push(cpu, cpu->PC / 256);
    push(cpu, cpu->PC % 256 + 3);
    cpu->PBR = mem_fetch(cpu, inc_addr(inc_addr(addr)));
    cpu->PC = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr));
}

//test BITs
void BIT(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
        two_bytes ans = (two_bytes){cpu->C.h & (v >> 8), cpu->C.l & (v & 0x00FF)}; 
        cpu->P.N = ans.h >> 7;
        cpu->P.V = (ans.h >> 6) & 0b01;//P.V = bit 14
        cpu->P.Z = (ans.h == 0) && (ans.l == 0);
        return;
    }
    byte v = mem_fetch(cpu, addr);
    byte ans = cpu->C.l & v;
    cpu->P.N = ans >> 7;
    cpu->P.V = (ans >> 6) & 0b01;//P.V = bit 6
    cpu->P.Z = ans == 0;
}

//ROtate Left
void ROL(CPUState* cpu, address addr){
    if (cpu->P.M == 0){
        two_bytes val = (two_bytes){mem_fetch(cpu, inc_addr(addr)), mem_fetch(cpu, addr)};
        byte old_carry = cpu->P.E;
        cpu->P.E = val.h >> 7;
        val = (two_bytes){(val.h << 1) + (val.l >> 7), (val.l << 1) + old_carry};
        cpu->P.N = val.h >> 7;
        cpu->P.Z = (val.h == 0) && (val.l == 0);
        mem_set(cpu, val.l, addr);
        mem_set(cpu, val.h, inc_addr(addr));
        return;
    }
    byte val = mem_fetch(cpu, addr);
    byte old_carry = cpu->P.E;
    cpu->P.E = val >> 7;
    val = (val << 1) + old_carry;
    cpu->P.N = val >> 7;
    cpu->P.Z = val == 0;
    mem_set(cpu, val, addr);
}

//PuLl P
void PLP(CPUState* cpu, address addr){
    byte P = pull(cpu);
    if (cpu->P.M == 0){
        cpu->P = byte_to_status(P);
        return;
    }
    P |= 0b00100000; //set emulation bit
    cpu->P = byte_to_status(P);
}

//ROtate Left Accumulator
void ROLA(CPUState* cpu, address addr){
    if (cpu->P.M == 0){
        byte old_carry = cpu->P.E;
        cpu->P.E = cpu->C.h >> 7;
        cpu->C = (two_bytes){(cpu->C.h << 1) + (cpu->C.l >> 7), (cpu->C.l << 1) + old_carry};
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte old_carry = cpu->P.E;
    cpu->P.E = cpu->C.l >> 7;
    cpu->C.l = (cpu->C.l << 1) + old_carry;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//PuLl D
void PLD(CPUState* cpu, address addr){
    cpu->D = pull(cpu) + pull(cpu) * 256;
}

//Branch if MInus
void BMI(CPUState* cpu, address addr){
    char offset = mem_fetch(cpu, addr);
    if(cpu->P.N == 1){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//SEt Carry flag
void SEC(CPUState* cpu, address addr){
    cpu->P.E = 1;
}

//Decrement Accumulator
void DECA(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        cpu->C = sub_2_1(cpu->C, 0x01);
        return;
    }
    cpu->C.l--;
}

//Transfer Stack to aCcumulator
void TSC(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        cpu->C = (two_bytes){cpu->S.h, cpu->S.l};
        return;
    }
    cpu->C.l = cpu->S.l;
}

//ReTurn from Interrupt
void RTI(CPUState* cpu, address addr){
    byte P = pull(cpu);
    cpu->P = byte_to_status(P);
    cpu->PC = pull(cpu) + pull(cpu) * 256;
    if (cpu->P.M == 0){//16-bit
        cpu->PBR = pull(cpu);
    }
}

//Exclusive OR with accumulator
void EOR(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
        cpu->C = separate_bytes(v ^ bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(cpu, addr);
    cpu->C.l = cpu->C.l ^ v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//William David Mensch (2 byte NOP)
void WDM(CPUState* cpu, address addr){
    //
}

//MoVe block Positive
void MVP(CPUState* cpu, address addr){
    byte dest_bank = mem_fetch(cpu, addr);
    byte src_bank = mem_fetch(cpu, inc_addr(addr));
    while (!((cpu->C.h == 0xff) && (cpu->C.l == 0xff))){
        mem_set(cpu, mem_fetch(cpu, (address){src_bank, cpu->X}), (address){dest_bank, cpu->Y});
        cpu->X--;
        cpu->Y--;
        cpu->C = sub_2_1(cpu->C, 0x01);
    }
}

//Logical Shift Right
void LSR(CPUState* cpu, address addr){
    if (cpu->P.M == 0){
        two_bytes val = (two_bytes){mem_fetch(cpu, inc_addr(addr)), mem_fetch(cpu, addr)};
        val = (two_bytes){(val.h >> 1) + cpu->P.E * 128, (val.l >> 1) + (val.h & 0b00000001)};
        cpu->P.E = val.l & 0b00000001;//lowest bit goes into carry
        mem_set(cpu, val.l, addr);
        mem_set(cpu, val.h, inc_addr(addr));
        cpu->P.N = val.h >> 7; //highest bit is sign bit
        cpu->P.Z = (val.h == 0) && (val.l == 0);
        return;
    }
    byte val = mem_fetch(cpu, addr);
    val = val >> 1;
    cpu->P.E = val & 0b00000001;
    mem_set(cpu, val, addr);
    cpu->P.N = val >> 7;
    cpu->P.Z = (val == 0);
}

//PusH Accumulator
void PHA(CPUState* cpu, address addr){
    push(cpu, cpu->C.h);
    push(cpu, cpu->C.l);
}

//Logical Shift Right Accumulator
void LSRA(CPUState* cpu, address addr){
    if (cpu->P.M == 0){
        cpu->C = (two_bytes){(cpu->C.h >> 1) + cpu->P.E * 128, (cpu->C.l >> 1) + (cpu->C.h & 0b00000001)};
        cpu->P.E = cpu->C.l & 0b00000001;//lowest bit goes into carry
        cpu->P.N = cpu->C.h >> 7; //highest bit is sign bit
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    cpu->C.l = cpu->C.l >> 1;
    cpu->P.E = cpu->C.l & 0b00000001;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = (cpu->C.l == 0);
}

//PusH program BanK
void PHK(CPUState* cpu, address addr){
    push(cpu, cpu->PBR);
}

//JuMP
void JMP(CPUState* cpu, address addr){
    cpu->PC = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
}

//JMP Long addr
void JMPL(CPUState* cpu, address addr){
    cpu->PC = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
    cpu->PBR = mem_fetch(cpu, inc_addr(inc_addr(addr)));
}

//Branch if oVerflow Clear
void BVC(CPUState* cpu, address addr){
    char offset = mem_fetch(cpu, addr);
    if(cpu->P.V == 0){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//MoVe block Negative
void MVN(CPUState* cpu, address addr){
    byte dest_bank = mem_fetch(cpu, addr);
    byte src_bank = mem_fetch(cpu, inc_addr(addr));
    while (!((cpu->C.h == 0xff) && (cpu->C.l == 0xff))){
        mem_set(cpu, mem_fetch(cpu, (address){src_bank, cpu->X}), (address){dest_bank, cpu->Y});
        cpu->X++;
        cpu->Y++;
        cpu->C = sub_2_1(cpu->C, 0x01);
    }
}

//CLear Interrupt flag
void CLI(CPUState* cpu, address addr){
    cpu->P.I = 0;
}

//PusH Y
void PHY(CPUState* cpu, address addr){
    push(cpu, cpu->Y / 256);
    push(cpu, cpu->Y % 256);
}

//Transdfer aCcumulator to D
void TCD(CPUState* cpu, address addr){
    cpu->D = cpu->C.h * 256 + cpu->C.l;
}

//ReTurn from Subroutine
void RTS(CPUState* cpu, address addr){

}

//ADd with Carry
void ADC(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
        cpu->C = separate_bytes(v + bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(cpu, addr);
    cpu->C.l = cpu->C.l + v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//Push Effective pc Relative
void PER(CPUState* cpu, address addr){
    uint16_t sum = cpu->PC + mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 256;
    push(cpu, sum / 256);
    push(cpu, sum % 256);
}

//STore Zero
void STZ(CPUState* cpu, address addr){
    mem_set(cpu, 0x00, addr);
    if(cpu->P.M == 0) mem_set(cpu, 0x00, inc_addr(addr));
}

//ROtate Right
void ROR(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        two_bytes val = (two_bytes){mem_fetch(cpu, inc_addr(addr)), mem_fetch(cpu, addr)};
        byte old_carry = cpu->P.E;
        cpu->P.E = val.l & 0b00000001;
        val = (two_bytes){(val.h >> 1) + old_carry * 128, (val.l >> 1) + (val.h & 0b00000001)};
        mem_set(cpu, val.l, addr);
        mem_set(cpu, val.h, inc_addr(addr));
        return;
    }
    byte val = mem_fetch(cpu, addr);
    byte old_carry = cpu->P.E;
    cpu->P.E = val & 0b00000001;
    val = (val >> 1) + old_carry * 128;
    mem_set(cpu, val, addr);
}

//PuLl Accumulator
void PLA(CPUState* cpu, address addr){
    cpu->C.l = pull(cpu);
    cpu->C.h = pull(cpu);
    cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
    cpu->P.N = cpu->C.h >> 7;
}

//ROtate Right Accumulator
void RORA(CPUState* cpu, address addr){
    if(cpu->P.M == 0){//16-bit
        byte old_carry = cpu->P.E;
        cpu->P.E = cpu->C.l & 0b00000001;
        cpu->C = (two_bytes){(cpu->C.h >> 1) + old_carry * 128, (cpu->C.l >> 1) + (cpu->C.h & 0b00000001)};
        return;
    }
    byte old_carry = cpu->P.E;
    cpu->P.E = cpu->C.l & 0b00000001;
    cpu->C.l = (cpu->C.l >> 1) + old_carry * 128;
}

//ReTurn from subroutine Long
void RTL(CPUState* cpu, address addr){
    cpu->PC = pull(cpu) + pull(cpu) * 256 + 1;
    cpu->PBR = pull(cpu);
}

const instr_data instructions[] = {
    {s, 7, 2, BRK},
    {dxi, 6, 2, ORA},
    {s, 7, 2, COP},
    {ds, 4, 2, ORA},
    {d, 5, 2, TSB},
    {d, 3, 2, ORA},
    {d, 5, 2, ASL},
    {dli, 6, 2, ORA},
    {s, 3, 1, PHP},
    {imm, 2, 2, ORA},
    {nil, 2, 1, ASLA},
    {s, 4, 1, PHD},
    {a, 6, 3, TSB},
    {a, 4, 3, ORA},
    {a, 6, 3, ASL},
    {al, 5, 2, ORA},
    
    {r, 2, 2, BPL},
    {diy, 5, 2, ORA},
    {di, 5, 2, ORA},
    {dsiy, 7, 2, ORA},
    {d, 5, 2, TRB},
    {dx, 4, 2, ORA},
    {dx, 6, 2, ASL},
    {dliy, 6, 2, ORA},
    {nil, 2, 1, CLC},
    {ay, 4, 3, ORA},
    {nil, 2, 1, INCA},
    {nil, 2, 1, TCS},
    {a, 6, 3, TRB},
    {ax, 4, 3, ORA},
    {ax, 7, 3, ASL},
    {alx, 5, 4, ORA},
    
    {a, 6, 3, JSR},
    {dxi, 6, 2, AND},
    {al, 8, 4, JSL},
    {ds, 4, 2, AND},
    {d, 3, 2, BIT},
    {d, 3, 2, AND},
    {d, 5, 2, ROL},
    {dli, 6, 2, AND},
    {s, 4, 1, PLP},
    {imm, 2, 2, AND},
    {nil, 2, 1, ROLA},
    {s, 5, 1, PLD},
    {a, 4, 3, BIT},
    {a, 4, 3, AND},
    {a, 6, 3, ROL},
    {al, 5, 4, AND},

    {r, 2, 2, BMI},
    {diy, 5, 2, AND},
    {di, 5, 2, AND},
    {dsiy, 7, 2, AND},
    {dx, 4, 2, BIT},
    {dx, 4, 2, AND},
    {dx, 6, 2, ROL},
    {dliy, 6, 2, AND},
    {nil, 2, 1, SEC},
    {ay, 4, 3, AND},
    {nil, 2, 1, DECA},
    {nil, 2, 1, TSC},
    {ax, 4, 3, BIT},
    {ax, 4, 3, AND},
    {ax, 7, 3, ROL},
    {ax, 5, 4, AND},

    {s, 7, 1, RTI},
    {dxi, 6, 2, EOR},
    {nil, 2, 2, WDM},
    {ds, 4, 2, EOR},
    {xyc, 7, 3, MVP},
    {d, 3, 2, EOR},
    {d, 5, 2, LSR},
    {dli, 6, 2, EOR},
    {s, 3, 1, PHA},
    {imm, 2, 2, EOR},
    {nil, 2, 1, LSRA},
    {s, 3, 1, PHK},
    {a, 3, 3, JMP},
    {a, 4, 3, EOR},
    {a, 6, 3, LSR},
    {al, 5, 4, EOR},

    {r, 2, 2, BVC},
    {diy, 5, 2, EOR},
    {di, 5, 2, EOR},
    {dsiy, 7, 2, EOR},
    {xyc, 7, 3, MVN},
    {dx, 4, 2, EOR},
    {dx, 6, 2, LSR},
    {dliy, 6, 2, EOR},
    {nil, 2, 1, CLI},
    {ay, 4, 3, EOR},
    {s, 3, 1, PHY},
    {nil, 2, 1, TCD},
    {al, 4, 4, JMPL},
    {ax, 4, 3, EOR},
    {ax, 7, 3, LSR},
    {alx, 5, 4, EOR},

    {s, 6, 1, RTS},
    {dxi, 6, 2, ADC},
    {s, 6, 3, PER},
    {ds, 4, 2, ADC},
    {d, 3, 2, STZ},
    {d, 3, 2, ADC},
    {d, 5, 2, ROR},
    {dli, 6, 2, ADC},
    {s, 4, 1, PLA},
    {imm, 2, 2, ADC},
    {nil, 2, 1, RORA},
    {s, 6, 1, RTL},
    {ai, 5, 3, JMP},
    {a, 4, 3, ADC},
    {a, 6, 3, ROR},
    {al, 5, 4, ADC},
    
    /*
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    */
};

/*run instruction
returns 0 for success; 1: somehow illegal opcode; 2:...*/
char run_instr_cpu(CPUState* cpu){
    byte instr = mem_fetch(cpu, (address){cpu->PBR, cpu->PC});
    char buf[5];
    byte_to_str(instr, buf);
    PrintXY(1, 1, buf, 0, 0);
    put_disp();
    cpu->PC++;
    instr_data instruction = instructions[instr];
    address addr = instruction.addr_func(cpu);
    instruction.instr_func(cpu, addr);
    cpu->PC += instruction.read_bytes;
    cpu->expected_time += instruction.cycle_count * CYCLE_TIME;
    return 0;
}

void cycle_cpu(CPUState* cpu){
    run_instr_cpu(cpu);
    while(cpu->current_time < cpu->expected_time){//if ahead, wait
        cpu->current_time = RTC_GetTicks();
    }
}