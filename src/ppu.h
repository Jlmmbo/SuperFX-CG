#define PPU_SCREEN_WIDTH 340
#define PPU_SCREEN_HEIGHT 261

void set_SNES_pixel(byte x, byte y, color_t color){
    ((color_t*)GetVRAMAddress())[y * LCD_WIDTH_PX + x] = color;
}

Tile fetch_tile(PPU* ppu, byte bg, uint16_t tile_index, byte four_bpp){
    Tile tile = {{{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},}};
    uint16_t nametable_addr;
    if(bg == 0){
        nametable_addr = (ppu->BG12NBA & 0b00001111) << 12;
    } else if(bg == 1){
        nametable_addr = (ppu->BG12NBA >> 4) << 12;
    } else if(bg == 2){
        nametable_addr -=(ppu->BG34NBA & 0b00001111) << 12;
    } else {
        nametable_addr = (ppu->BG34NBA >> 4) << 12;
    }
    for (byte i; i<8; i++){
        for(byte j; j<8; j++){
            byte px = get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i], j) + get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i + 8], j) * 2 + (four_bpp ? (get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i + 16], j) * 4 + get_bit(ppu->VRAM[nametable_addr + tile_index * 8 + i + 24], j) * 8) : 0);
            tile.data[i][j] = px;
        }
    }
    return tile;
}

void PPU_dot(PPU* ppu){
    //Evaluate priorities for this dot
    uint16_t tilemap_addr;
    byte flip;
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

    tilemap_addr = (ppu->BG1SC >> 2) * 0x800;
    tilemap_entry = ppu->VRAM[tilemap_addr + (((ppu->v_cntr + ppu->BG1VOFS) / 8) * (32 + get_bit(ppu->BG1SC, 0) * 32)) + ((ppu->h_cntr + ppu->BG1HOFS) / 8)];
    byte bg1_priority = (tilemap_entry & 0b0010000000000000) >> 13;
    byte bg1_palette = (tilemap_entry & 0b0001110000000000) >> 10;
    tile_index = tilemap_entry & 0b00000011111111;
    flip = (tilemap_entry & 0b1100000000000000) >> 14;
    tile = fetch_tile(ppu, 0, tile_index, 1);
    byte bg1_px = tile.data[(flip & 0b10) ? (8-(ppu->BG1VOFS % 8)) : (ppu->BG1VOFS % 8)][(flip & 0b01) ? (8 - (ppu->BG1HOFS % 8)) : (ppu->BG1HOFS % 8)];

    tilemap_addr = (ppu->BG2SC >> 2) * 0x800;
    tilemap_entry = ppu->VRAM[tilemap_addr + (((ppu->v_cntr + ppu->BG2VOFS) / 8) * (32 + get_bit(ppu->BG2SC, 0) * 32)) + ((ppu->h_cntr + ppu->BG2HOFS) / 8)];
    byte bg2_priority = (tilemap_entry & 0b0010000000000000) >> 13;
    byte bg2_palette = (tilemap_entry & 0b0001110000000000) >> 10;
    tile_index = tilemap_entry & 0b00000011111111;
    flip = (tilemap_entry & 0b1100000000000000) >> 14;
    tile = fetch_tile(ppu, 1, tile_index, 1);
    byte bg2_px = tile.data[(flip & 0b10) ? (8-(ppu->BG2VOFS % 8)) : (ppu->BG2VOFS % 8)][(flip & 0b01) ? (8 - (ppu->BG2HOFS % 8)) : (ppu->BG2HOFS % 8)];

    tilemap_addr = (ppu->BG3SC >> 2) * 0x800;
    tilemap_entry = ppu->VRAM[tilemap_addr + (((ppu->v_cntr + ppu->BG3VOFS) / 8) * (32 + get_bit(ppu->BG3SC, 0) * 32)) + ((ppu->h_cntr + ppu->BG3HOFS) / 8)];
    byte bg3_priority = (tilemap_entry & 0b0010000000000000) >> 13;
    byte bg3_palette = (tilemap_entry & 0b0001110000000000) >> 10;
    tile_index = tilemap_entry & 0b00000011111111;
    flip = (tilemap_entry & 0b1100000000000000) >> 14;
    tile = fetch_tile(ppu, 2, tile_index, 1);
    byte bg3_px = tile.data[(flip & 0b10) ? (8-(ppu->BG3VOFS % 8)) : (ppu->BG3VOFS % 8)][(flip & 0b01) ? (8 - (ppu->BG3HOFS % 8)) : (ppu->BG3HOFS % 8)];

    tilemap_addr = (ppu->BG4SC >> 2) * 0x800;
    tilemap_entry = ppu->VRAM[tilemap_addr + (((ppu->v_cntr + ppu->BG4VOFS) / 8) * (32 + get_bit(ppu->BG4SC, 0) * 32)) + ((ppu->h_cntr + ppu->BG4HOFS) / 8)];
    byte bg4_priority = (tilemap_entry & 0b0010000000000000) >> 13;
    byte bg4_palette = (tilemap_entry & 0b0001110000000000) >> 10;
    tile_index = tilemap_entry & 0b00000011111111;
    flip = (tilemap_entry & 0b1100000000000000) >> 14;
    tile = fetch_tile(ppu, 3, tile_index, 1);
    byte bg4_px = tile.data[(flip & 0b10) ? (8-(ppu->BG4VOFS % 8)) : (ppu->BG4VOFS % 8)][(flip & 0b01) ? (8 - (ppu->BG4HOFS % 8)) : (ppu->BG4HOFS % 8)];


    byte px = ppu->BGCOLOR;
    byte palette = 0;
    //placeholder, find a better way to implement priorities for multiple modes
    if(bg1_priority){
        if (bg1_px == 0){
            if(bg2_priority){
                if (bg2_px == 0){
                    if(bg3_priority){
                        if (bg3_px == 0){
                            if(bg4_priority){
                                if (bg4_px != 0) {
                                    px = bg4_px;
                                    palette = bg4_palette;
                                }
                            }
                        } else {
                            px = bg3_px;
                            palette = bg3_palette;
                        }
                    }
                } else {
                    px = bg2_px;
                    palette = bg2_palette;
                }
            }
        } else {
            px = bg1_px;
            palette = bg1_palette;
        }
    }

    color_t color = ppu->VRAM[ppu->CGADD + palette * 16 + px];
    color = ((color & 0b0111110000000000) << 1) + ((color & 0b0000001111100000) << 1) + (color & 0b0000000000011111);//convert from rgb555 to rgb565
    set_SNES_pixel(ppu->h_cntr, ppu->v_cntr, color);

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

void generate_frame(CPUState* cpu){
    PPU ppu = cpu->ppu;
    for(ppu.v_cntr = 0; ppu.v_cntr < PPU_SCREEN_HEIGHT; ppu.v_cntr++){
        for(ppu.h_cntr = 0; ppu.h_cntr < PPU_SCREEN_WIDTH; ppu.h_cntr++){
            PPU_dot(&ppu);
        }
        cpu->IRQ = 1;//H_blank
    }
    cpu->IRQ = 1;//V_blank
}