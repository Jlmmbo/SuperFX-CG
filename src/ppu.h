#define PPU_SCREEN_WIDTH 340
#define PPU_SCREEN_HEIGHT 261

void set_SNES_pixel(byte x, byte y, color_t color){
    color_t* VRAM = (color_t*)GetVRAMAddress();
    VRAM += y * LCD_WIDTH_PX + x;
    *VRAM = color;
}

Tile fetch_tile(CPUState* cpu, byte bg, uint16_t tile_index, byte four_bpp){
    PPU* ppu = &(cpu->ppu);
    Tile tile = {{{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},}};
    uint16_t nametable_addr;
    if(bg == 0){
        nametable_addr = (BG12NBA & 0b00001111) << 12;
    } else if(bg == 1){
        nametable_addr = (BG12NBA >> 4) << 12;
    } else if(bg == 2){
        nametable_addr -=(BG34NBA & 0b00001111) << 12;
    } else {
        nametable_addr = (BG34NBA >> 4) << 12;
    }
    for (byte i; i<8; i++){
        for(byte j; j<8; j++){
            byte px = get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i], j) + get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i + 8], j) * 2 + (four_bpp ? (get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i + 16], j) * 4 + get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i + 24], j) * 8) : 0);
            tile.data[i][j] = px;
        }
    }
    return tile;
}

void PPU_dot(CPUState* cpu){
    PPU* ppu = &(cpu->ppu);
    //Evaluate priorities for this dot
    
    uint16_t tilemap_addr;
    //byte flip;
    uint16_t tile_index;
    uint16_t tilemap_entry;
    Tile tile;
    //uint16_t tilemap_entry;
    //VHPL LLTT   TTTT TTTT
    //|||| ||||   |||| ||||
    //|||| ||++---++++-++++- Tile index
    //|||+-++--------------- Palette
    //||+------------------- Priority
    //++-------------------- vertical or horizontal

    //bgn_px = wrong thing !! deal w/ bitplanes!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!111

    tilemap_addr = (BG1SC >> 2) * 0x800;
    tilemap_entry = ppu->VRAM[tilemap_addr + (((ppu->v_cntr + BG1VOFS) / 8) * (32 + get_bit(BG1SC, 0) * 32)) + ((ppu->h_cntr + BG1HOFS) / 8)];
    byte bg1_priority = (tilemap_entry & 0b0010000000000000) >> 13;
    byte bg1_palette = (tilemap_entry & 0b0001110000000000) >> 10;
    tile_index = tilemap_entry & 0b00000011111111;
    //flip = (tilemap_entry & 0b1100000000000000) >> 14;
    tile = fetch_tile(cpu, 0, tile_index, 1);
    //byte bg1_px = tile.data[(flip & 0b10) ? (8-(BG1VOFS % 8)) : (BG1VOFS % 8)][(flip & 0b01) ? (8 - (BG1HOFS % 8)) : (BG1HOFS % 8)];
    byte bg1_px = tile.data[BG1VOFS % 8][BG1HOFS % 8];

    byte px = bg1_px;// = BGCOLOR;
    byte palette = bg1_palette;
    //placeholder, find a better way to implement priorities for multiple modes
    /*if(bg1_priority){
            px = bg1_px;
            palette = bg1_palette;
    }*/

    color_t color = ppu->VRAM[CGADD + palette * 16 + px];
    color = ((color & 0b0111110000000000) << 1) + ((color & 0b0000001111100000) << 1) + (color & 0b0000000000011111);//convert from rgb555 to rgb565
    set_SNES_pixel(ppu->h_cntr, ppu->v_cntr, color);

    //Apply window masks
    //Perform color math (if enabled)
    //Output final pixel (256 visible dots only)

    //Update H/V counters

    //Trigger scanline events if appropriate
}

void generate_frame(CPUState* cpu){
    uint16_t i;
    PPU* ppu = &(cpu->ppu);
    for(ppu->v_cntr = 0; ppu->v_cntr < PPU_SCREEN_HEIGHT; ppu->v_cntr++){
        for(ppu->h_cntr = 0; ppu->h_cntr < PPU_SCREEN_WIDTH; ppu->h_cntr++){
            PPU_dot(cpu);
            cycle_cpu(cpu);
        }
        if(keydownlast(KEY_PRGM_MENU)) pause_menu_ui();
        put_disp();
        cpu->IRQ = 1;//H_blank
        cycle_cpu(cpu);
        cpu->IRQ = 0;
        for(i = 0; i < 350; i++){
            cycle_cpu(cpu);
        }
    }
    cpu->IRQ = 1;//V_blank
    cycle_cpu(cpu);
    cpu->IRQ = 0;
    for(i = 0; i < 350; i++){
        cycle_cpu(cpu);
    }
}