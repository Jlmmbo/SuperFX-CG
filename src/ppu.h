#define PPU_SCREEN_WIDTH 340
#define PPU_SCREEN_HEIGHT 261

typedef struct Tile{
   byte data[8][8];
}Tile;

byte get_ppu_reg(CPUState* cpu, byte id){
    return mem_fetch(cpu, (address){0x00, 0x2100 + id});
}

Tile fetch_tile(unsigned short h, unsigned short v){
    return (Tile){{
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
    }};//placeholder
}

void cycle_PPU(CPUState* cpu){
    //For each BG layer:
    color_t pixle;
    for(unsigned char bg_layer = 0; bg_layer < cpu->ppu.num_bg_layers; bg_layer++){
        //tile = fetch(x+xscroll, y+yscroll)
        //apply scroll offsets and flipping
        //px = tile[x+xscroll, y+yscroll] (and flipping)
        //palette index
        //px = transparent? yes: skip
        //rgb color
    }

    //For each sprite active this line:
        //advance OBJ pattern fetch if needed
        //shift OBJ pixel output

    //Evaluate priorities for this dot
    //Apply window masks
    //Perform color math (if enabled)
    //Output final pixel (256 visible dots only)

    //Update H/V counters
    if(cpu->ppu.h_cntr >= PPU_SCREEN_WIDTH){
        cpu->ppu.h_cntr = 0;
        cpu->ppu.v_cntr++;
    }
    if(cpu->ppu.v_cntr >= PPU_SCREEN_HEIGHT){
        cpu->ppu.v_cntr = 0;
    }

    //Trigger scanline events if appropriate
}