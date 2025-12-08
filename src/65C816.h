#define CYCLE_TIME (62)
//~62 home ticks per 65c816 tick

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


void push(byte val){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0) mem_set(val, (cpu->S.h << 8) + cpu->S.l);
    else mem_set(val, cpu->S.l);
    cpu->S = sub_2_1(cpu->S, 0x01);//descending stack
}

byte pull(){
    CPUState* cpu = &CPU;
    cpu->S = add_2_1(cpu->S, 0x01);//??????????? if in emulation mode, does S.h decrement on page wrap?
    if (cpu->P.M == 0) return mem_fetch((cpu->S.h << 8) + cpu->S.l);
    return mem_fetch(cpu->S.l);
}

address inc_addr(address addr){
    return (addr + 1) & 0xFFFFFF;
}

address add_addr(address addr, address amt){
    return (addr + amt) & 0xFFFFFF;
}


address mem_get_addr(address addr, byte rom_mode){
    if(rom_mode == 0){//LoROM
        addr &= 0x7F7FFF;//ignore bank high bit and page high bit
    } else if(rom_mode == 1){//HiROM
        addr &= 0x3FFFFF;//ignore highest 2 bits of bank
    } else if(rom_mode == 5){//ExHiROM
        addr &= 0xBFFFF;//ignore 2nd highest bit, highest is supposed to be inverted, but who cares?
    }
    return addr;
}

/*Fetch memory from address in bank.
 If bank has not already been allocated, calls sys_malloc()*/
byte mem_fetch(address addr){
    CPUState* cpu = &CPU;
    addr = mem_get_addr(addr, cpu->rom_mode);
    byte bank = addr >> 16;
    if((bank < 0x3F) && ((addr & 0x00FFFF) > 0x8000)){//is a rom sector
        return cpu->rom->raw[(bank & 0x7F) * 0x8000 + (addr & 0x7FFF)];
    }
    if(cpu->mem[addr >> 16]==NULL){
        cpu->mem[addr >> 16] = (byte*)sys_malloc(65536 * sizeof(byte));
    }
    if((addr >= 0x2100) || (addr <= 0x21FF)){//MMIO regs
        if(addr == 0x213B){//CGDATAREAD
            if(cgram_byte){
                cgram_byte = 0;
                return ppu_CGRAM[CGADD] >> 8;
            } else {
                cgram_byte = 1;
                return ppu_CGRAM[CGADD] & 0xFF;
            }
        }
        if(addr == 0x213A){//VMDATAHREAD
            uint16_t val = vram_latch >> 8;
            if (VMAIN >> 7){
                vram_latch = ppu_VRAM[(VMADDH << 8) | VMADDL];
                VMADDH += ((VMADDL + VMAIN_inc_amt[VMAIN & 0b11]) < VMADDL) ? 1 : 0;
                VMADDL += VMAIN_inc_amt[VMAIN & 0b11];
            }
            return val;
        }
        if(addr == 0x2139){//VMDATALREAD
            uint16_t val = vram_latch & 0xFF;
            if (!(VMAIN >> 7)){
                vram_latch = ppu_VRAM[(VMADDH << 8) | VMADDL];
                VMADDH += ((VMADDL + VMAIN_inc_amt[VMAIN & 0b11]) < VMADDL) ? 1 : 0;
                VMADDL += VMAIN_inc_amt[VMAIN & 0b11];
            }
            return val;
        }
    }
    return cpu->mem[addr >> 16][addr & 0xFFFF];
}

/*set memory at address in bank.
 If bank has not already been allocated, will call sys_malloc()*/
char mem_set(byte value, address addr){
    CPUState* cpu = &CPU;
    addr = mem_get_addr(addr, cpu->rom_mode);
    if(((addr >> 16) < 0x3F) && ((addr & 0x00FFFF) > 0x8000)){//is a rom sector
        return 0;
    }
    if(cpu->mem[addr >> 16]==NULL){
        cpu->mem[addr >> 16] = (byte*)sys_malloc(65536 * sizeof(byte));
    }
    if(addr > 0x2100 && addr < 0x21FF){//MMIO regs
        if(addr == 0x2118 || addr == 0x2119){//VMDATA
            ppu_VRAM[(VMADDH << 8) + VMADDL] = (VMDATAH << 8) + VMDATAL;
            if ((addr == 0x2118 && !(VMAIN >> 7)) || (addr == 0x2119 && (VMAIN >> 7))){
                VMADDH += ((VMADDL + VMAIN_inc_amt[VMAIN & 0b11]) < VMADDL) ? 1 : 0;
                VMADDL += VMAIN_inc_amt[VMAIN & 0b11];
            }
        }
        if(addr == 0x2122){//CGDATA
            if(cgram_byte){
                ppu_CGRAM[CGADD] = (CGDATA << 8) | cgram_latch;
                cgram_byte = 0;
            } else {
                cgram_latch = CGDATA;
                cgram_byte = 1;
            }
        }
        if(addr == 0x2121) cgram_byte = 0;//CGADD
    }
    //todo: if addr would be rom, do nothing
    cpu->mem[addr >> 16][addr & 0xFFFF] = value;
    //ppu regs at 2100-213f: also set cpu->ppu.-----
    return 0;
}

/*
Will write rom to proper memory banks/regions, 
and allocate used non-rom banks (e.g. ram, sram, i/o &c.)
also mirror proper banks*/
char write_rom(Rom rom){
    CPUState* cpu = &CPU;
    cpu->rom_mode = 0x00;

    if (rom.size > 0x007fc0 && rom.raw[0x007fc0] == 0){
        cpu->rom_mode = 0;//LoROM
    }
    else if (rom.size > 0x00fc0 && rom.raw[0x00ffc0] == 1){
        cpu->rom_mode = 1;//HiROM
    }
    else if (rom.size > 0x40ffc0 && rom.raw[0x40ffc0] == 5){
        cpu->rom_mode = 5;//ExHiROM
    }
    //error_msg("rom mode found");
    //address rom_offset = 0;

    //todo: initialize ram banks
    /*for (byte bank = 0x00; bank <= 0x7D; bank++) {
        for (uint16_t i = 0x8000; i <= 0xFFF; i++) {
            //disp_msg(byte_to_str(rom_offset >> 8, tmp));
            if (rom_offset < rom.size) {
                mem_set(rom.raw[rom_offset++], (bank << 16) + i);
            //} else {
                //cpu->mem[bank][i] = 0xFF;  // open bus
            }
        }
    }*/
    return 0;
}

/*start up cpu, initialize regs, set load interrupt vector etc.*/
void init_cpu(Rom rom){
    CPUState* cpu = &CPU;
    PPUState* ppu = &PPU;
    disp_msg("allocating space...");
    for (int i = 0; i < 256; i++){
        for (int j = 0; j < 65536; j++){
            cpu->mem[i][j] = 0;
        }
    }
    for (int i = 0; i < 256; i++){//double check all banks are allocated
        mem_fetch(i << 16);
    }

    cpu->E = 1;
    
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
    
    cpu->rom = &rom;
    char err = write_rom(rom);
    if (err){//failed to allocate
        Bdisp_AllClr_VRAM();
        PrintXY(1, 1, "  GET MORE SPACE!!", 0, 0);
        return;
    }

    //cpu->PC = (cpu->mem[0][0xFFFD] << 8) + cpu->mem[0][0xFFFC];
    cpu->PC = (mem_fetch(0x00fffd) << 8) + mem_fetch(0x00fffc);

    ppu->h_cntr = 0;
    ppu->v_cntr = 0;
    mem_fetch((VMADDH << 8) + VMADDL);//just to allocate the bank
    ppu_VRAM = sys_malloc(sizeof(byte) * 65536);
    ppu_CGRAM = sys_malloc(sizeof(byte) * 512);
    ppu_TILES = sys_malloc(sizeof(Tile) * 4096);
    cgram_byte = 1;
    cgram_latch = 0;
    vram_latch = 0;

    MDMAEN = 0;
    HDMAEN = 0;

    for(byte i = 0; i < 8; i++){
        DMAP(i) = 0xff;
        BBADn(i) = 0xff;
        A1TnH(i) = 0xff;
        A1TnL(i) = 0xff;
        A1Bn(i) = 0xff;
        DASnH(i) = 0xff;
        DASnL(i) = 0xff;
        DASBn(i) = 0xff;
        A2AnH(i) = 0xff;
        A2AnL(i) = 0xff;
        NLTRn(i) = 0xff;
    }
}

//absolute
address a(){
    CPUState* cpu = &CPU;
    address addr = (cpu->PBR << 16) + cpu->PC;
    return (cpu->DBR << 16) + mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
}

//absolute indexed indirect
address axi(){
    CPUState* cpu = &CPU;
    address addr = (cpu->PBR << 16) + cpu->PC;
    return mem_fetch(addr) + mem_fetch((inc_addr(addr)) << 8) + cpu->X;
}

//absolute indexed X
address ax(){
    CPUState* cpu = &CPU;
    address addr = (cpu->PBR << 16) + cpu->PC;
    return (cpu->DBR << 16) + mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8) + cpu->X;
}

//absolute indexed Y
address ay(){
    CPUState* cpu = &CPU;
    address addr = (cpu->PBR << 16) + cpu->PC;
    return (cpu->DBR << 16) + mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8) + cpu->Y;
}

//absolute indirect
address ai(){
    CPUState* cpu = &CPU;
    address addr = (cpu->PBR << 16) + cpu->PC;
    return mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
}

//absolute long indexed
address alx(){
    CPUState* cpu = &CPU;
    address addr = (cpu->PBR << 16) + cpu->PC;
    return mem_fetch((inc_addr(inc_addr(addr))) << 16) + mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8) + cpu->X;
}

//absolute long
address al(){
    CPUState* cpu = &CPU;
    address addr = (cpu->PBR << 16) + cpu->PC;
    return mem_fetch((inc_addr(inc_addr(addr))) << 16) + mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
}

//block move
address xyc(){
    CPUState* cpu = &CPU;
    return (cpu->DBR << 16) + mem_fetch((cpu->PBR << 16) + cpu->PC);
}

//direct indexed indirect
address dxi(){
    CPUState* cpu = &CPU;
    return (cpu->DBR << 16) + cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC) + cpu->X;
}

//direct indexed X
address dx(){
    CPUState* cpu = &CPU;
    return cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC) + cpu->X;
}

//direct indexed Y
address dy(){
    CPUState* cpu = &CPU;
    return cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC) + cpu->Y;
}

//direct indirect indexed
address diy(){
    CPUState* cpu = &CPU;
    return add_addr((cpu->DBR << 16) + cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC), cpu->Y);
}

//direct long indirecte indexed
address dliy(){
    CPUState* cpu = &CPU;
    return add_addr(cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC), cpu->Y);
}

//direct long indirect
address dli(){
    CPUState* cpu = &CPU;
    return cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC);
}

//direct indirect
address di(){
    CPUState* cpu = &CPU;
    return (cpu->DBR << 16) + cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC);
}

//direct
address d(){
    CPUState* cpu = &CPU;
    return cpu->D + mem_fetch((cpu->PBR << 16) + cpu->PC);
}

// immediate
address imm(){
    CPUState* cpu = &CPU;
    return (cpu->PBR << 16) + cpu->PC;
}

//no address
address nil(){
    return 0;
}

//relative long
address rl(){
    CPUState* cpu = &CPU;
    return (cpu->PBR << 16) + cpu->PC;//////////////////////////////////////////////////////////////////////////////
}

//relative
address r(){
    CPUState* cpu = &CPU;
    return (cpu->PBR << 16) + cpu->PC;//////////////////////////////////////////////////////////////////////////////
}

//stack
address s(){
    CPUState* cpu = &CPU;
    return (cpu->S.h << 8) + cpu->S.l;
}

//direct stack
address ds(){
    CPUState* cpu = &CPU;
    return add_addr((cpu->S.h << 8) + cpu->S.l, mem_fetch((cpu->PBR << 16) + cpu->PC));
}

//direct stack indirect indexed
address dsiy(){
    CPUState* cpu = &CPU;
    return add_addr((cpu->DBR << 16) + (cpu->S.h << 8) + cpu->S.l + mem_fetch((cpu->PBR << 16) + cpu->PC), cpu->Y);
}

void disp_cpu_stats(){
    CPUState* cpu = &CPU;
    Bdisp_AllClr_VRAM();
    char* tmp = "                               ";
    PrintCXY(0, 1, "C", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(40, 1, "X", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(80, 1, "Y", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(120, 1, "S", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(160, 1, "D", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(200, 1, "E", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(240, 1, "IR", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(280, 1, "NI", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(320, 1, "PB", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(360, 1, "DB", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(0, 70, "PC", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(40, 70, "RS", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(80, 70, "rm", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(120, 70, "P", 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(160, 70, "IN", 0, -1, 0, COLOR_WHITE, 1, 0);

    PrintCXY(0, 20, byte_to_str(cpu->C.h, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(0, 40, byte_to_str(cpu->C.l, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(40, 20, byte_to_str(cpu->X >> 8, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(40, 40, byte_to_str(cpu->X & 0xFF, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(80, 20, byte_to_str(cpu->Y >> 8, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(80, 40, byte_to_str(cpu->Y & 0xFF, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(120, 20, byte_to_str(cpu->S.h, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(120, 40, byte_to_str(cpu->S.l, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(160, 20, byte_to_str(cpu->D >> 8, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(160, 40, byte_to_str(cpu->D & 0xFF, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(200, 20, byte_to_str(cpu->E, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(240, 20, byte_to_str(cpu->IRQ, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(280, 20, byte_to_str(cpu->NMI, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(320, 20, byte_to_str(cpu->PBR, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(360, 20, byte_to_str(cpu->DBR, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(0, 90, byte_to_str(cpu->PC >> 8, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(0, 110, byte_to_str(cpu->PC & 0xFF, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(40, 90, byte_to_str(cpu->RES, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(80, 90, byte_to_str(cpu->rom_mode, tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(120, 90, byte_to_str(status_to_byte(cpu->P), tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    PrintCXY(160, 90, byte_to_str(mem_fetch((cpu->PBR << 16) + cpu->PC), tmp), 0, -1, 0, COLOR_WHITE, 1, 0);
    //GetKey(NULL);
}

//BReaK
void BRK(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        push(cpu->PBR);
        push(cpu->PC >> 8);
        push((cpu->PC & 0xFF) + 2);
        push(status_to_byte(cpu->P));
        cpu->PC = mem_fetch(0x00ffe6) + (mem_fetch(0x00ffe7) << 8);
        cpu->P.I = 1;
    } else {
        push(cpu->PC >> 8);
        push((cpu->PC & 0xFF) + 2);
        push(status_to_byte(cpu->P));
        cpu->PC = mem_fetch(0x00fffe) + (mem_fetch(0x00ffff) << 8);
        cpu->P.I = 1;
        cpu->P.X = 1;
    }
    cpu->P.D = 0;
}

//OR with Accumulator
void ORA(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
        cpu->C = separate_bytes(v | bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(addr);
    cpu->C.l = cpu->C.l | v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//CO-Processor
void COP(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        push(cpu->PBR);
        cpu->PBR = 0;
        push(cpu->PC >> 8);
        push((cpu->PC & 0xFF) + 2);
        push(status_to_byte(cpu->P));
        cpu->P.I = 1;
        cpu->P.D = 0;
        return;
    }
    push(cpu->PC >> 8);
    push(cpu->PC & 0xFF);
    push(status_to_byte(cpu->P));
    cpu->P.I = 1;
    cpu->P.D = 0;
}

//Test and Set Bits
void TSB(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        mem_set(mem_fetch(addr) | cpu->C.l, addr);
        mem_set(mem_fetch(inc_addr(addr)) | cpu->C.h, inc_addr(addr));
        return;
    }
    mem_set(cpu->C.l | mem_fetch(addr), addr);
}

//Arithmetic Shift Left
void ASL(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        two_bytes val = (two_bytes){mem_fetch(addr), mem_fetch(inc_addr(addr))};
        cpu->P.E = val.h >> 7;//highest bit goes into carry
        val = add_2_2(val, val);//multiply by 2
        mem_set(val.h, addr);
        mem_set(val.l, inc_addr(addr));
        cpu->P.N = val.h >> 7; //highest bit is sign bit
        cpu->P.Z = (val.h == 0) && (val.l == 0);
        return;
    }
    byte val = mem_fetch(addr);
    cpu->P.E = val >> 7;
    val = val << 1;
    mem_set(val, addr);
    cpu->P.N = val >> 7;
    cpu->P.Z = (val == 0);
}

//PusH P
void PHP(address addr){
    CPUState* cpu = &CPU;
    cpu->P.X = 1;//idk if break flag is set
    push(status_to_byte(cpu->P));
}

//Arithmetic Shift Left Accumulator
void ASLA(address addr){
    CPUState* cpu = &CPU;
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
void PHD(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        push(cpu->D >> 8);
        push(cpu->D & 0xFF);
        return;
    }
    push(cpu->D & 0xFF);
}

//Branch if PLus
void BPL(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.N == 0){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//Test and Reset Bits
void TRB(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        two_bytes val = (two_bytes){mem_fetch(inc_addr(addr)), mem_fetch(addr)};
        mem_set((val.l ^ cpu->C.l) & val.l, addr);//Set bits in C = clear bits in mem
        mem_set((val.h ^ cpu->C.h) & val.h, inc_addr(addr));//somehow (val XOR C) & val does this
        return;
    }
    byte val = mem_fetch(addr);
    mem_set((val ^ cpu->C.l) & val, addr);
}

//CLear Carry
void CLC(address addr){
    CPUState* cpu = &CPU;
    cpu->P.E = 0;
}

//INCrement Accumulator
void INCA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        cpu->C = add_2_1(cpu->C, 0x01);
        return;
    }
    cpu->C.l++;
}

//Transfer aCcumulator to Stack
void TCS(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        cpu->S = (two_bytes){cpu->C.h, cpu->C.l};
        return;
    }
    cpu->S.l = cpu->C.l;
}

//Jump to SubRoutine
void JSR(address addr){
    CPUState* cpu = &CPU;
    push(cpu->PC >> 8);
    push(cpu->PC & 0xFF);
    cpu->PC = mem_fetch(addr) + mem_fetch(inc_addr(addr));
}

//AND with Accumulator
void AND(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
        cpu->C = separate_bytes(v & bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(addr);
    cpu->C.l = cpu->C.l & v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//Jump to Subroutine Long
void JSL(address addr){
    CPUState* cpu = &CPU;
    push(cpu->PBR);
    push(cpu->PC >> 8);
    push((cpu->PC & 0xFF) + 3);
    cpu->PBR = mem_fetch(inc_addr(inc_addr(addr)));
    cpu->PC = mem_fetch(addr) + mem_fetch(inc_addr(addr));
}

//test BITs
void BIT(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
        two_bytes ans = (two_bytes){cpu->C.h & (v >> 8), cpu->C.l & (v & 0x00FF)}; 
        cpu->P.N = ans.h >> 7;
        cpu->P.V = (ans.h >> 6) & 0b01;//P.V = bit 14
        cpu->P.Z = (ans.h == 0) && (ans.l == 0);
        return;
    }
    byte v = mem_fetch(addr);
    byte ans = cpu->C.l & v;
    cpu->P.N = ans >> 7;
    cpu->P.V = (ans >> 6) & 0b01;//P.V = bit 6
    cpu->P.Z = ans == 0;
}

//ROtate Left
void ROL(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        two_bytes val = (two_bytes){mem_fetch(inc_addr(addr)), mem_fetch(addr)};
        byte old_carry = cpu->P.E;
        cpu->P.E = val.h >> 7;
        val = (two_bytes){(val.h << 1) + (val.l >> 7), (val.l << 1) + old_carry};
        cpu->P.N = val.h >> 7;
        cpu->P.Z = (val.h == 0) && (val.l == 0);
        mem_set(val.l, addr);
        mem_set(val.h, inc_addr(addr));
        return;
    }
    byte val = mem_fetch(addr);
    byte old_carry = cpu->P.E;
    cpu->P.E = val >> 7;
    val = (val << 1) + old_carry;
    cpu->P.N = val >> 7;
    cpu->P.Z = val == 0;
    mem_set(val, addr);
}

//PuLl P
void PLP(address addr){
    CPUState* cpu = &CPU;
    byte P = pull(cpu);
    if (cpu->P.M == 0){//16-bit
        cpu->P = byte_to_status(P);
        return;
    }
    P |= 0b00110000; //set emulation bit
    cpu->E = 1;
    cpu->P = byte_to_status(P);
}

//ROtate Left Accumulator
void ROLA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
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
void PLD(address addr){
    CPUState* cpu = &CPU;
    cpu->D = pull(cpu) + (pull(cpu) << 8);
}

//Branch if MInus
void BMI(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.N == 1){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//SEt Carry flag
void SEC(address addr){
    CPUState* cpu = &CPU;
    cpu->P.E = 1;
}

//Decrement Accumulator
void DECA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        cpu->C = sub_2_1(cpu->C, 0x01);
        return;
    }
    cpu->C.l--;
}

//Transfer Stack to aCcumulator
void TSC(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        cpu->C = (two_bytes){cpu->S.h, cpu->S.l};
        return;
    }
    cpu->C.l = cpu->S.l;
}

//ReTurn from Interrupt
void RTI(address addr){
    CPUState* cpu = &CPU;
    byte P = pull(cpu);
    cpu->P = byte_to_status(P);
    cpu->PC = pull(cpu) + (pull(cpu) << 8);
    if (cpu->P.M == 0){//16-bit
        cpu->PBR = pull(cpu);
    }
}

//Exclusive OR with accumulator
void EOR(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
        cpu->C = separate_bytes(v ^ bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(addr);
    cpu->C.l = cpu->C.l ^ v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//William David Mensch (2 byte NOP)
void WDM(address addr){
    //
}

//MoVe block Positive
void MVP(address addr){
    CPUState* cpu = &CPU;
    byte dest_bank = mem_fetch(addr);
    byte src_bank = mem_fetch(inc_addr(addr));
    while (!((cpu->C.h == 0xff) && (cpu->C.l == 0xff))){
        mem_set(mem_fetch((src_bank << 16) + cpu->X), (dest_bank << 16) + cpu->Y);
        cpu->X--;
        cpu->Y--;
        cpu->C = sub_2_1(cpu->C, 0x01);
    }
}

//Logical Shift Right
void LSR(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        two_bytes val = (two_bytes){mem_fetch(inc_addr(addr)), mem_fetch(addr)};
        val = (two_bytes){(val.h >> 1) + cpu->P.E * 128, (val.l >> 1) + (val.h & 0b00000001)};
        cpu->P.E = val.l & 0b00000001;//lowest bit goes into carry
        mem_set(val.l, addr);
        mem_set(val.h, inc_addr(addr));
        cpu->P.N = val.h >> 7; //highest bit is sign bit
        cpu->P.Z = (val.h == 0) && (val.l == 0);
        return;
    }
    byte val = mem_fetch(addr);
    val = val >> 1;
    cpu->P.E = val & 0b00000001;
    mem_set(val, addr);
    cpu->P.N = val >> 7;
    cpu->P.Z = (val == 0);
}

//PusH Accumulator
void PHA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0) push(cpu->C.h);
    push(cpu->C.l);
}

//Logical Shift Right Accumulator
void LSRA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
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

//PusH program banK
void PHK(address addr){
    CPUState* cpu = &CPU;
    push(cpu->PBR);
}

//JuMP
void JMP(address addr){
    CPUState* cpu = &CPU;
    cpu->PC = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
}

//JMP Long addr
void JMPL(address addr){
    CPUState* cpu = &CPU;
    cpu->PC = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
    cpu->PBR = mem_fetch(inc_addr(inc_addr(addr)));
}

//Branch if oVerflow Clear
void BVC(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.V == 0){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//MoVe block Negative
void MVN(address addr){
    CPUState* cpu = &CPU;
    byte dest_bank = mem_fetch(addr);
    byte src_bank = mem_fetch(inc_addr(addr));
    while (!((cpu->C.h == 0xff) && (cpu->C.l == 0xff))){
        mem_set(mem_fetch((src_bank << 16) + cpu->X), (dest_bank << 16) + cpu->Y);
        cpu->X++;
        cpu->Y++;
        cpu->C = sub_2_1(cpu->C, 0x01);
    }
}

//CLear Interrupt flag
void CLI(address addr){
    CPUState* cpu = &CPU;
    cpu->P.I = 0;
}

//PusH Y
void PHY(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.X == 0) push(cpu->Y >> 8);
    push(cpu->Y & 0xFF);
}

//Transdfer aCcumulator to D
void TCD(address addr){
    CPUState* cpu = &CPU;
    cpu->D = (cpu->C.h << 8) + cpu->C.l;
}

//ReTurn from Subroutine
void RTS(address addr){
    CPUState* cpu = &CPU;
    cpu->PC = pull(cpu) + (pull(cpu) << 8);
}

//ADd with Carry
void ADC(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
        cpu->C = separate_bytes(v + bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(addr);
    cpu->C.l = cpu->C.l + v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//Push Effective pc Relative
void PER(address addr){
    CPUState* cpu = &CPU;
    uint16_t sum = cpu->PC + mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
    push(sum >> 8);
    push(sum & 0xFF);
}

//STore Zero
void STZ(address addr){
    CPUState* cpu = &CPU;
    mem_set(0x00, addr);
    if(cpu->P.M == 0) mem_set(0x00, inc_addr(addr));
}

//ROtate Right
void ROR(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        two_bytes val = (two_bytes){mem_fetch(inc_addr(addr)), mem_fetch(addr)};
        byte old_carry = cpu->P.E;
        cpu->P.E = val.l & 0b00000001;
        val = (two_bytes){(val.h >> 1) + old_carry * 128, (val.l >> 1) + (val.h & 0b00000001)};
        mem_set(val.l, addr);
        mem_set(val.h, inc_addr(addr));
        return;
    }
    byte val = mem_fetch(addr);
    byte old_carry = cpu->P.E;
    cpu->P.E = val & 0b00000001;
    val = (val >> 1) + old_carry * 128;
    mem_set(val, addr);
}

//PuLl Accumulator
void PLA(address addr){
    CPUState* cpu = &CPU;
    cpu->C.l = pull(cpu);
    if (cpu->P.M == 0){//16-bit
        cpu->C.h = pull(cpu);
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        cpu->P.N = cpu->C.h >> 7;
        return;
    }
    cpu->P.Z = cpu->C.l == 0;
    cpu->P.N = cpu->C.l >> 7;
}

//ROtate Right Accumulator
void RORA(address addr){
    CPUState* cpu = &CPU;
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
void RTL(address addr){
    CPUState* cpu = &CPU;
    cpu->PC = pull(cpu) + (pull(cpu) << 8) + 1;
    cpu->PBR = pull(cpu);
}

//Branch oVerflow Set
void BVS(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.V == 1){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//SEt Interrupt flag
void SEI(address addr){
    CPUState* cpu = &CPU;
    cpu->P.I = 1;
}

//PuLl Y
void PLY(address addr){
    CPUState* cpu = &CPU;
    cpu->Y = pull(cpu) + (cpu->P.X ? 0 : (pull(cpu) << 8));
}

//Transfer D to aCcumulator
void TDC(address addr){
    CPUState* cpu = &CPU;
    cpu->C = (two_bytes){cpu->D >> 8, cpu->D %  256};
}

//BRanch Always
void BRA(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    cpu->PC = cpu->PC + (offset - 128);
}

//STore Accumulator
void STA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        mem_set(cpu->C.l, addr);
        mem_set(cpu->C.h, inc_addr(addr));
        return;
    }
    mem_set(cpu->C.l, addr);
}

//BRanch Long
void BRL(address addr){
    CPUState* cpu = &CPU;
    uint16_t offset = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
    cpu->PC = cpu->PC + (offset - 32768);
}

//STore Y
void STY(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.X == 0){//16-bit
        mem_set(cpu->Y >> 8, addr);
        mem_set(cpu->Y & 0xFF, inc_addr(addr));
        return;
    }
    mem_set(cpu->Y >> 8, addr);
}

//STore X
void STX(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.X == 0){//16-bit
        mem_set(cpu->X >> 8, addr);
        mem_set(cpu->X & 0xFF, inc_addr(addr));
        return;
    }
    mem_set(cpu->X >> 8, addr);
}

//DECrement Y
void DEY(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.X == 0){//16-bit
        cpu->Y--;
        cpu->P.N = cpu->Y >> 15;
    } else {
        cpu->Y = (cpu->Y >> 8) + ((cpu->Y & 0xFF) - 1);
        cpu->P.N = cpu->Y >> 7;
    }
    cpu->P.Z = cpu->Y == 0;
}

//Transfer X to Accumulator
void TXA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        cpu->C = (two_bytes){cpu->X >> 8, cpu->X & 0xFF};
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    cpu->C.l = cpu->X & 0xFF;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//PusH data Bank
void PHB(address addr){
    CPUState* cpu = &CPU;
    push(cpu->DBR);
}

//Branch Carry Clear
void BCC(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.E == 0){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//Transfer Y to Accumulator
void TYA(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        cpu->C = (two_bytes){cpu->Y >> 8, cpu->Y & 0xFF};
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    cpu->C.l = cpu->Y & 0xFF;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//Transfer X to Stack
void TXS(address addr){
    CPUState* cpu = &CPU;
    if ((cpu->P.M == 0) && (cpu->P.X == 0)){//does M define the size of S??
        cpu->S = (two_bytes){cpu->X >> 8, cpu->X & 0xFF};
    } else if (cpu->P.M == 0){
        cpu->S = (two_bytes){0, cpu->X & 0xFF};
    } else {
        cpu->S.l = cpu->X & 0xFF;
    }
}

//Transfer X to Y
void TXY(address addr){
    CPUState* cpu = &CPU;
    cpu->Y = cpu->X;
    cpu->P.Z = cpu->Y == 0;
    cpu->P.N = cpu->Y >> (cpu->P.X ? 7 : 15);
}

//LoaD to Y
void LDY(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        cpu->Y = mem_fetch(addr) + mem_fetch(inc_addr(addr));
        cpu->P.N = cpu->Y >> 15;
    } else {
        cpu->Y = mem_fetch(addr);
        cpu->P.N = cpu->Y >> 7;
    }
    cpu->P.Z = cpu->Y == 0;
}

//LoaD to Accumulator
void LDA(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        cpu->C = (two_bytes){mem_fetch(inc_addr(addr)), mem_fetch(addr)};
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l== 0);
    } else {
        cpu->C.l = mem_fetch(addr);
        cpu->P.N = cpu->C.l >> 7;
        cpu->P.Z = cpu->C.l == 0;
    }
}

//LoaD to X
void LDX(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        cpu->X = mem_fetch(addr) + mem_fetch(inc_addr(addr));
        cpu->P.N = cpu->X >> 15;
    } else {
        cpu->X = mem_fetch(addr);
        cpu->P.N = cpu->X >> 7;
    }
    cpu->P.Z = cpu->X == 0;
}

//Transfer Accumulator to Y
void TAY(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.X == 0){//16-bit
        cpu->Y = (cpu->C.h << 8) + cpu->C.l;
        cpu->P.N = cpu->Y >> 15;
    } else {
        cpu->Y = cpu->C.l;
        cpu->P.N = cpu->Y >> 7;
    }
    cpu->P.Z = cpu->Y == 0;
}

//Transfer Accumulator to X
void TAX(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.X == 0){//16-bit
        cpu->X = (cpu->C.h << 8) + cpu->C.l;
        cpu->P.N = cpu->X >> 15;
    } else {
        cpu->X = cpu->C.l;
        cpu->P.N = cpu->X >> 7;
    }
    cpu->P.Z = cpu->X == 0;
}

//PuLl data Bank
void PLB(address addr){
    CPUState* cpu = &CPU;
    cpu->DBR = pull(cpu);
}

//Branch Carry Set
void BCS(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.E == 1){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//CLear oVerflow flag
void CLV(address addr){
    CPUState* cpu = &CPU;
    cpu->P.V = 0;
}

//Transfer Stack to X
void TSX(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.X == 0){//16-bit
        cpu->X = (cpu->S.h << 8) + cpu->S.l;
        cpu->P.N = cpu->X >> 15;
    } else {
        cpu->X = cpu->S.l;
        cpu->P.N = cpu->X >> 7;
    }
    cpu->P.Z = cpu->X == 0;
}

//Transfer Y to X
void TYX(address addr){
    CPUState* cpu = &CPU;
    cpu->X = cpu->Y;
    cpu->P.N = cpu->X >> (cpu->P.X ? 7 : 15);
    cpu->P.Z = cpu->X == 0;
}

//ComPare memory with Y
void CPY(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.X == 0){//16-bit
        uint16_t val = cpu->Y - mem_fetch(addr);
        cpu->P.Z = val == 0;
        cpu->P.N = val >> 15;
        cpu->P.E = val <= cpu->Y;
        return;
    }
    byte val = cpu->Y - mem_fetch(addr);
    cpu->P.Z = val == 0;
    cpu->P.N = val >> 7;
    cpu->P.E = val <= cpu->Y;
}

//CoMPare memory with Accumulator
void CMP(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        uint16_t C = (cpu->C.h << 8) + cpu->C.l;
        uint16_t val = C - mem_fetch(addr);
        cpu->P.Z = val == 0;
        cpu->P.N = val >> 15;
        cpu->P.E = val <= C;
        return;
    }
    byte val = cpu->C.l - mem_fetch(addr);
    cpu->P.Z = val == 0;
    cpu->P.N = val >> 7;
    cpu->P.E = val <= cpu->C.l;
}

//REset P bits
void REP(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        byte P_byte = status_to_byte(cpu->P);
        cpu->P = byte_to_status(((P_byte ^ mem_fetch(addr)) & P_byte));
        return;
    }
    byte P_byte = status_to_byte(cpu->P);
    cpu->P = byte_to_status((P_byte ^ mem_fetch(addr)) & P_byte & 0b11001111);//can't change M or X flags if in emulation mode
}

//DECrement memory
void DEC(address addr){
    CPUState* cpu = &CPU;
    uint16_t val;
    if (cpu->P.X == 0){//16-bit
        val = mem_fetch(addr) + mem_fetch(inc_addr(addr)) - 1;
        cpu->P.N = cpu->Y >> 15;
        mem_set(val & 0xFFFF, addr);
        mem_set((val >> 16), inc_addr(addr));
    } else {
        val = mem_fetch(addr) - 1;
        cpu->P.N = cpu->Y >> 7;
    }
    cpu->P.Z = val == 0;
}

//INcrement Y
void INY(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.X == 0){//16-bit
        cpu->Y++;
        cpu->P.N = cpu->Y >> 15;
    } else {
        cpu->Y = (cpu->Y >> 8) + ((cpu->Y & 0xFF) + 1);
        cpu->P.N = cpu->Y >> 7;
    }
    cpu->P.Z = cpu->Y == 0;
}

//DEcrement X
void DEX(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.X == 0){//16-bit
        cpu->X--;
        cpu->P.N = cpu->X >> 15;
    } else {
        cpu->X = (cpu->X >> 8) + ((cpu->X & 0xFF) - 1);
        cpu->P.N = cpu->X >> 7;
    }
    cpu->P.Z = cpu->X == 0;
}

//WAit for Interrupt
void WAI(address addr){
    CPUState* cpu = &CPU;
    if (!(cpu->NMI | cpu->IRQ)) {
        cpu->PC--;
        keyupdate();
        if(keydownlast(KEY_CTRL_MENU)) pause_menu_ui();
        //disp_cpu_stats(cpu);
    }
}

//Branch Not Equal
void BNE(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.Z == 0){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//Push Effective Indirect address
void PEI(address addr){
    CPUState* cpu = &CPU;
   push(mem_fetch((cpu->PBR << 16) + mem_fetch((cpu->PBR << 16) + cpu->PC) + cpu->D));
}

//CLear Decimal flag
void CLD(address addr){
    CPUState* cpu = &CPU;
    cpu->P.D = 0;
}

//PusH X
void PHX(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.X == 0) push(cpu->X >> 8);
    push(cpu->X & 0xFF);
}

//SToP the clock
void STP(address addr){
    CPUState* cpu = &CPU;
    while (cpu->RES);
}

//
void CPX(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.X == 0){//16-bit
        uint16_t val = cpu->X - mem_fetch(addr);
        cpu->P.Z = val == 0;
        cpu->P.N = val >> 15;
        cpu->P.E = val <= cpu->X;
        return;
    }
    byte val = cpu->X - mem_fetch(addr);
    cpu->P.Z = val == 0;
    cpu->P.N = val >> 7;
    cpu->P.E = val <= cpu->X;
}

//
void SBC(address addr){
    CPUState* cpu = &CPU;
    if(cpu->P.M == 0){//16-bit
        uint16_t v = mem_fetch(addr) + (mem_fetch(inc_addr(addr)) << 8);
        cpu->C = separate_bytes(v - bytes_to_int(cpu->C.h, cpu->C.l));
        cpu->P.N = cpu->C.h >> 7;
        cpu->P.Z = (cpu->C.h == 0) && (cpu->C.l == 0);
        return;
    }
    byte v = mem_fetch(addr);
    cpu->C.l = cpu->C.l - v;
    cpu->P.N = cpu->C.l >> 7;
    cpu->P.Z = cpu->C.l == 0;
}

//
void SEP(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.M == 0){//16-bit
        byte P_byte = status_to_byte(cpu->P);
        cpu->P = byte_to_status(P_byte | mem_fetch(addr));
        return;
    }
    byte P_byte = status_to_byte(cpu->P);
    cpu->P = byte_to_status((P_byte | mem_fetch(addr)) & P_byte & 0b11001111);//can't change M or X flags if in emulation mode
}

//
void INC(address addr){
    CPUState* cpu = &CPU;
    uint16_t val;
    if (cpu->P.X == 0){//16-bit
        val = mem_fetch(addr) + mem_fetch(inc_addr(addr)) + 1;
        cpu->P.N = cpu->Y >> 15;
        mem_set(val & 0xFFFF, addr);
        mem_set((val >> 16), inc_addr(addr));
    } else {
        val = mem_fetch(addr) + 1;
        cpu->P.N = cpu->Y >> 7;
    }
    cpu->P.Z = val == 0;
}

//INcrement X
void INX(address addr){
    CPUState* cpu = &CPU;
    if (cpu->P.X == 0){//16-bit
        cpu->X++;
        cpu->P.N = cpu->X >> 15;
    } else {
        cpu->X = (cpu->X >> 8) + ((cpu->X & 0xFF) + 1);
        cpu->P.N = cpu->X >> 7;
    }
    cpu->P.Z = cpu->X == 0;
}

//No OPeration
void NOP(address addr){
    //
}

//eXchange B and A
void XBA(address addr){
    CPUState* cpu = &CPU;
    byte temp = cpu->C.h;
    cpu->C.h = cpu->C.l;
    cpu->C.l = temp;
}

//Branch if EQual
void BEQ(address addr){
    CPUState* cpu = &CPU;
    byte offset = mem_fetch(addr);
    if(cpu->P.Z == 1){
        cpu->PC = cpu->PC + (offset - 128);
        return;
    }
    cpu->PC++;
}

//Push Effective Address
void PEA(address addr){
    push(mem_fetch(inc_addr(addr)));
    push(mem_fetch(addr));
}

//SEt Decimal flag
void SED(address addr){
    CPUState* cpu = &CPU;
    cpu->P.D = 1;
}

//Pull X
void PLX(address addr){
    CPUState* cpu = &CPU;
    cpu->X = pull(cpu) + (cpu->P.X ? 0 : (pull(cpu) << 8));
}

//Transfer C to E
void XCE(address addr){
    CPUState* cpu = &CPU;
    byte temp = cpu->P.E;
    cpu->P.E = cpu->E;
    cpu->E = temp;
}

const instr_data instructions[] = {
    {s, 2, BRK},
    {dxi, 2, ORA},
    {s, 2, COP},
    {ds, 2, ORA},
    {d, 2, TSB},
    {d, 2, ORA},
    {d, 2, ASL},
    {dli, 2, ORA},
    {s, 1, PHP},
    {imm, 2, ORA},
    {nil, 1, ASLA},
    {s, 1, PHD},
    {a, 3, TSB},
    {a, 3, ORA},
    {a, 3, ASL},
    {al, 2, ORA},
    
    {r, 2, BPL},
    {diy, 2, ORA},
    {di, 2, ORA},
    {dsiy, 2, ORA},
    {d, 2, TRB},
    {dx, 2, ORA},
    {dx, 2, ASL},
    {dliy, 2, ORA},
    {nil, 1, CLC},
    {ay, 3, ORA},
    {nil, 1, INCA},
    {nil, 1, TCS},
    {a, 3, TRB},
    {ax, 3, ORA},
    {ax, 3, ASL},
    {alx, 4, ORA},
    
    {a, 3, JSR},
    {dxi, 2, AND},
    {al, 4, JSL},
    {ds, 2, AND},
    {d, 2, BIT},
    {d, 2, AND},
    {d, 2, ROL},
    {dli, 2, AND},
    {s, 1, PLP},
    {imm, 2, AND},
    {nil, 1, ROLA},
    {s, 1, PLD},
    {a, 3, BIT},
    {a, 3, AND},
    {a, 3, ROL},
    {al, 4, AND},

    {r, 2, BMI},
    {diy, 2, AND},
    {di, 2, AND},
    {dsiy, 2, AND},
    {dx, 2, BIT},
    {dx, 2, AND},
    {dx, 2, ROL},
    {dliy, 2, AND},
    {nil, 1, SEC},
    {ay, 3, AND},
    {nil, 1, DECA},
    {nil, 1, TSC},
    {ax, 3, BIT},
    {ax, 3, AND},
    {ax, 3, ROL},
    {ax, 4, AND},

    {s, 1, RTI},
    {dxi, 2, EOR},
    {nil, 2, WDM},
    {ds, 2, EOR},
    {xyc, 3, MVP},
    {d, 2, EOR},
    {d, 2, LSR},
    {dli, 2, EOR},
    {s, 1, PHA},
    {imm, 2, EOR},
    {nil, 1, LSRA},
    {s, 1, PHK},
    {a, 3, JMP},
    {a, 3, EOR},
    {a, 3, LSR},
    {al, 4, EOR},

    {r, 2, BVC},
    {diy, 2, EOR},
    {di, 2, EOR},
    {dsiy, 2, EOR},
    {xyc, 3, MVN},
    {dx, 2, EOR},
    {dx, 2, LSR},
    {dliy, 2, EOR},
    {nil, 1, CLI},
    {ay, 3, EOR},
    {s, 1, PHY},
    {nil, 1, TCD},
    {al, 4, JMPL},
    {ax, 3, EOR},
    {ax, 3, LSR},
    {alx, 4, EOR},

    {s, 1, RTS},
    {dxi, 2, ADC},
    {s, 3, PER},
    {ds, 2, ADC},
    {d, 2, STZ},
    {d, 2, ADC},
    {d, 2, ROR},
    {dli, 2, ADC},
    {s, 1, PLA},
    {imm, 2, ADC},
    {nil, 1, RORA},
    {s, 1, RTL},
    {ai, 3, JMP},
    {a, 3, ADC},
    {a, 3, ROR},
    {al, 4, ADC},

    {r, 2, BVS},
    {diy, 2, ADC},
    {di, 2, ADC},
    {dsiy, 2, ADC},
    {dx, 2, STZ},
    {dx, 2, ADC},
    {dx, 2, ROR},
    {dliy, 2, ADC},
    {nil, 1, SEI},
    {ay, 3, ADC},
    {s, 1, PLY},
    {nil, 1, TDC},
    {axi, 3, JMP},
    {ax, 3, ADC},
    {ax, 3, ROR},
    {alx, 4, ADC},
    
    {r, 2, BRA},
    {dx, 2, STA},
    {rl, 3, BRL},
    {ds, 2, STA},
    {d, 2, STY},
    {d, 2, STA},
    {d, 2, STX},
    {dli, 2, STA},
    {nil, 1, DEY},
    {imm, 2, BIT},
    {nil, 1, TXA},
    {s, 1, PHB},
    {a, 3, STY},
    {a, 3, STA},
    {a, 3, STX},
    {al, 3, STA},

    {r, 2, BCC},
    {diy, 2, STA},
    {di, 2, STA},
    {dsiy, 2, STA},
    {dx, 2, STY},
    {dx, 2, STA},
    {dy, 2, STX},
    {dliy, 2, STA},
    {nil, 1, TYA},
    {ay, 3, STA},
    {nil, 1, TXS},
    {nil, 1, TXY},
    {a, 3, STZ},
    {ax, 3, STA},
    {ax, 3, STZ},
    {alx, 4, STA},

    {imm, 2, LDY},
    {dxi, 2, LDA},
    {imm, 2, LDX},
    {ds, 2, LDA},
    {d, 2, LDY},
    {d, 2, LDA},
    {d, 2, LDX},
    {dli, 2, LDA},
    {nil, 1, TAY},
    {imm, 2, LDA},
    {nil, 1, TAX},
    {s, 1, PLB},
    {a, 3, LDY},
    {a, 3, LDA},
    {a, 3, LDX},
    {al, 4, LDA},
    
    {r, 2, BCS},
    {diy, 2, LDA},
    {di, 2, LDA},
    {dsiy, 2, LDA},
    {dx, 2, LDY},
    {dx, 2, LDA},
    {dy, 2, LDX},
    {dliy, 2, LDA},
    {nil, 1, CLV},
    {ay, 3, LDA},
    {nil, 1, TSX},
    {nil, 1, TYX},
    {ax, 3, LDY},
    {ax, 3, LDA},
    {ay, 3, LDX},
    {alx, 4, LDA},

    {imm, 2, CPY},
    {dxi, 2, CMP},
    {imm, 2, REP},
    {ds, 2, CMP},
    {d, 2, CPY},
    {d, 2, CMP},
    {d, 2, DEC},
    {dli, 2, CMP},
    {nil, 1, INY},
    {imm, 2, CMP},
    {nil, 1, DEX},
    {nil, 1, WAI},
    {a, 3, CPY},
    {a, 3, CMP},
    {a, 3, DEC},
    {al, 4, CMP},

    {r, 2, BNE},
    {diy, 2, CMP},
    {di, 2, CMP},
    {dsiy, 2, CMP},
    {nil, 2, PEI},
    {dx, 2, CMP},
    {dx, 2, DEC},
    {dliy, 2, CMP},
    {nil, 1, CLD},
    {ay, 3, CMP},
    {s, 1, PHX},
    {nil, 1, STP},
    {ai, 3, JMPL},
    {ax, 3, CMP},
    {ax, 3, DEC},
    {alx, 4, CMP},

    {imm, 2, CPX},
    {dxi, 2, SBC},
    {imm, 2, SEP},
    {ds, 2, SBC},
    {d, 2, CPX},
    {d, 2, SBC},
    {d, 2, INC},
    {dli, 2, SBC},
    {nil, 1, INX},
    {imm, 2, SBC},
    {nil, 1, NOP},
    {nil, 1, XBA},
    {a, 3, CPX},
    {a, 3, SBC},
    {a, 3, INC},
    {al, 4, SBC},
    
    {r, 2, BEQ},
    {diy, 2, SBC},
    {di, 2, SBC},
    {dsiy, 2, SBC},
    {imm, 3, PEA},
    {dx, 2, SBC},
    {dx, 2, INC},
    {dliy, 2, SBC},
    {nil, 1, SED},
    {ay, 3, SBC},
    {s, 1, PLX},
    {nil, 1, XCE},
    {axi, 3, JSR},
    {ax, 3, SBC},
    {ax, 3, INC},
    {alx, 4, SBC},
};

/*run instruction
returns 0 for success; 1: somehow illegal opcode; 2:...*/
char run_instr_cpu(){
    CPUState* cpu = &CPU;
    byte instr = mem_fetch((cpu->PBR << 16) + cpu->PC);
    //put_disp();
    cpu->PC++;
    instr_data instruction = instructions[instr];
    address addr = instruction.addr_func();
    instruction.instr_func(addr);
    cpu->PC += instruction.read_bytes;
    return 0;
}

void cycle_cpu(){
    run_instr_cpu();
    keyupdate();
    if(keydownlast(KEY_CTRL_MENU)) pause_menu_ui();
}

void update_controller_register(){
    mem_set((
    keydownlast(keybinds[0]) << 7 |//b
    keydownlast(keybinds[1]) << 6 |//y
    keydownlast(keybinds[2]) << 5 |//select
    keydownlast(keybinds[3]) << 4 |//start
    keydownlast(keybinds[4]) << 3 |//D-UP
    keydownlast(keybinds[5]) << 2 |//D-DOWN
    keydownlast(keybinds[6]) << 1 |//D-LEFT
    keydownlast(keybinds[7]) << 0 //D-RIGHT
    ), 0x4218);
    mem_set((
    keydownlast(keybinds[8]) << 7 |//A
    keydownlast(keybinds[9]) << 6 |//X
    keydownlast(keybinds[10]) << 5 |//L
    keydownlast(keybinds[11]) << 4 //R
    ), 0x4219);
    keyupdate();
    if(keydownlast(KEY_CTRL_MENU)) pause_menu_ui();
}