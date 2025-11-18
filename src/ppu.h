#define PPU_SCREEN_WIDTH 340
#define PPU_SCREEN_HEIGHT 261

typedef struct Tile{
   byte data[8][8];
}Tile;

byte get_ppu_reg(CPUState* cpu, byte id){
    return mem_fetch(cpu, (address){0x00, 0x2100 + id});
}

byte fetch_tile(byte bg_or_spite, byte index, unsigned short h, unsigned short v){
    
}

void cycle_PPU(PPU* ppu){
    //Evaluate priorities for this dot
    //Apply window masks
    //Perform color math (if enabled)
    //Output final pixel (256 visible dots only)

    //Update H/V counters
    if(ppu->h_cntr >= PPU_SCREEN_WIDTH){
        ppu->h_cntr = 0;
        ppu->v_cntr++;
    }
    if(ppu->v_cntr >= PPU_SCREEN_HEIGHT){
        ppu->v_cntr = 0;
    }

    //Trigger scanline events if appropriate
}