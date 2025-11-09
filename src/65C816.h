#define CYCLE_TIME (62)
//~62 home ticks per 65c816 tick


typedef struct {
    byte bank;
    uint16_t addr;
}address;

typedef struct{
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

typedef struct {
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

typedef struct {
    byte* raw;
    long int size:24;//long to ensure at least 24 bits reg. int may only be 16
}Rom;

typedef struct {
    address (*addr_func)(CPUState*);//function to calculate effective address
    byte cycle_count;
    byte read_bytes;//num of bytes read, includes opcode
    void (*instr_func)(CPUState*, address);//function to excecute instruction
}instr_data;

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



//increment program counter
uint16_t inc_PC(CPUState* cpu){
    cpu->PC++;
    return cpu->PC;
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
    return (address){cpu->DBR, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255};
}
//absolute indexed indirect
address axi(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){0x00, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255 + cpu->X};
}
//absolute indexed X
address ax(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){cpu->DBR, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255 + cpu->X};
}
//absolute indexed Y
address ay(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){cpu->DBR, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255 + cpu->Y};
}
//absolute indirect
address ai(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){0x00, mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255};
}
//absolute long indexed
address alx(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){mem_fetch(cpu, inc_addr(inc_addr(addr))), mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255 + cpu->X};
}
//absolute long
address al(CPUState* cpu){
    address addr = (address){cpu->PBR, cpu->PC};
    return (address){mem_fetch(cpu, inc_addr(inc_addr(addr))), mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255};
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
    return (address){0x00, cpu->S.h * 255 + cpu->S.l};
}
//direct stack
address ds(CPUState* cpu){
    return add_addr((address){0x00, cpu->S.h * 255 + cpu->S.l}, mem_fetch(cpu, (address){cpu->PBR, cpu->PC}));
}
//direct stack indirect indexed
address dsiy(CPUState* cpu){
    return add_addr((address){cpu->DBR, cpu->S.h * 255 + cpu->S.l + mem_fetch(cpu, (address){cpu->PBR, cpu->PC})}, cpu->Y);
}

void BRK(CPUState* cpu, address addr){

}

void ORA(CPUState* cpu, address addr){
    if(cpu->P.M){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255;
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

void COP(CPUState* cpu, address addr){
     
}

void TSB(CPUState* cpu, address addr){

}

void ASL(CPUState* cpu, address addr){

}

void PHP(CPUState* cpu, address addr){

}

void ASLA(CPUState* cpu, address addr){
    
}

void PHD(CPUState* cpu, address addr){

}

void BPL(CPUState* cpu, address addr){

}

void TRB(CPUState* cpu, address addr){
    
}

void CLC(CPUState* cpu, address addr){
    
}

void INCA(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        cpu->C = add_2_1(cpu->C, 0x01);
        return;
    }
    cpu->C.l++;
}

void TCS(CPUState* cpu, address addr){
    
}

void JSR(CPUState* cpu, address addr){

}

void AND(CPUState* cpu, address addr){
    if(cpu->P.M){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255;
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


void JSL(CPUState* cpu, address addr){

}

void BIT(CPUState* cpu, address addr){
    if(cpu->P.M){//16-bit
        uint16_t v = mem_fetch(cpu, addr) + mem_fetch(cpu, inc_addr(addr)) * 255;
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

void ROL(CPUState* cpu, address addr){

}

void PLP(CPUState* cpu, address addr){

}

void ROLA(CPUState* cpu, address addr){

}

void PLD(CPUState* cpu, address addr){

}

void BMI(CPUState* cpu, address addr){

}

void SEC(CPUState* cpu, address addr){
    cpu->P.E = 1;
}

void DECA(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        cpu->C = sub_2_1(cpu->C, 0x01);
        return;
    }
    cpu->C.l--;
}

void TSC(CPUState* cpu, address addr){
    if (cpu->P.M == 0){//16-bit
        cpu->C = (two_bytes){cpu->S.h, cpu->S.l};
        return;
    }
    cpu->C.l = cpu->S.l;
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
    inc_PC(cpu);
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