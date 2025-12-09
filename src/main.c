#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/heap.h>
#include <fxcg/file.h>
//#include <fxcg/rtc.h>
#include <fxcg/system.h>

int keybinds[12] = {KEY_CTRL_OPTN, KEY_CHAR_SQUARE, KEY_CTRL_F6, KEY_CTRL_F6, KEY_CTRL_UP, KEY_CTRL_DOWN, KEY_CTRL_LEFT, KEY_CTRL_RIGHT, KEY_CTRL_SHIFT, KEY_CTRL_ALPHA, KEY_CTRL_F1, KEY_CTRL_F2};//{B, Y, Select, Start, UP, DOWN, LEFT, RIGHT, A, X, L, R}

#include "main.h"

int main(void) {
    Bdisp_AllClr_VRAM();
    Bdisp_EnableColor(1);

    SetQuitHandler(quithandler);

    //optional overclock if needed
    //change_freq(PLL_18x);

    Rom rom;
    main_menu_ui(&rom);
    uint16_t vram_local[32768];
    PPU.VRAM = vram_local;
    uint16_t cgram_local[256];
    PPU.CGRAM = cgram_local;
    Tile tiles_local[4096];
    PPU.TILES = tiles_local;
    init_cpu(&rom);//rom gets loaded here

    cycle_cpu();

    //int k;
    while (1) {
        keyupdate();

        put_disp();
        if (keydownlast(KEY_CTRL_MENU)) break;
        
        update_controller_register(keybinds);

        generate_frame();
    }
    return 0;
}
