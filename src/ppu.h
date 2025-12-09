#define PPU_SCREEN_WIDTH 340
#define PPU_SCREEN_HEIGHT 261

void set_SNES_pixel(color_t color){
    color_t* VRAM = (color_t*)GetVRAMAddress();
    VRAM += PPU.v_cntr * LCD_WIDTH_PX + PPU.h_cntr;
    *VRAM = color;
}

void update_TILES(){
    error_msg("update tiles");
    Tile tile;
    for(uint16_t i = 0; i < 1024; i++){
        for(byte x = 0; x < 8; x++){
            for(byte y = 0; y < 8; y++){
                tile.data[y][x] = PPU.VRAM[((BG12NBA & 0b11111100) << 8/*or 10*/) + y * 32 + x];
            }
        }
        PPU.TILES[i] = tile;
    }
}

void PPU_dot(){
    PPUState* ppu = &PPU;
    
    uint16_t tilemap_addr;
    uint16_t tilemap_entry;

    //bgn_px = wrong thing !! deal w/ bitplanes!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!111

    tilemap_addr = (BG1SC >> 2) * 0x800;

    uint16_t map_width = (BG1SC & 0b10) ? 64 : 32;

    byte sx = (ppu->h_cntr + BG1HOFS);
    byte sy = (ppu->v_cntr + BG1VOFS);

    byte tile_x = sx >> 3;
    byte tile_y = sy >> 3;

    tilemap_entry = PPU.VRAM[tilemap_addr + tile_y * map_width + tile_x];
    uint16_t tile = tilemap_entry & 0x3FF;
    byte palette = (tilemap_entry >> 10) & 7;
    byte priority = (tilemap_entry >> 13) & 1;
    byte hflip = (tilemap_entry >> 14) & 1;
    byte vflip = (tilemap_entry >> 15) & 1;

    byte px = hflip ? (7 - (sx & 7)) : (sx & 7);
    byte py = vflip ? (7 - (sy & 7)) : (sy & 7);

    //byte color_index = ppu_VRAM[(BG12NBA & 0x0F) << 10]
    byte color_index = PPU.TILES[tile].data[py][px];

    uint16_t color = 0x00;//set to default bg color

    //placeholder, find a better way to implement priorities for multiple modes
    /*if(bg1_priority){
            px = bg1_px;
            palette = bg1_palette;
    }*/
    color_t palet[] = {0x0000, 0xF800, 0x07E0, 0x001F, 0xFFE0, 0x008F, 0xFFFF, 0x0000};
    //color = ppu_CGRAM[palette + color_index];
    color = palet[color_index];

    //color = ((color & 0b011111111100000) << 1) + (color & 0b0000000000011111);//convert from rgb555 to rgb565
    set_SNES_pixel(color);

    //Apply window masks
    //Perform color math (if enabled)
    //Output final pixel (256 visible dots only)

    //Update H/V counters

    //Trigger scanline events if appropriate
}

void generate_frame(){
    CPUState* cpu = &CPU;
    PPUState* ppu = &PPU;
    uint16_t i;
    for(ppu->v_cntr = 0; ppu->v_cntr < PPU_SCREEN_HEIGHT; ppu->v_cntr++){
        for(ppu->h_cntr = 0; ppu->h_cntr < PPU_SCREEN_WIDTH; ppu->h_cntr++){
            PPU_dot(cpu);
            cycle_cpu(cpu);
        }
        if(keydownlast(KEY_PRGM_MENU)) pause_menu_ui();
        put_disp();
        for(i = 0; i < 8; i++){//decrement all hdma scanline counters
            NLTRn(i)--;
        }
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