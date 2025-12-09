
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
