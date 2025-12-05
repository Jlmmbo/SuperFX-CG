#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/heap.h>
#include <fxcg/file.h>
#include <fxcg/rtc.h>
#include <fxcg/system.h>

#include "main.h"

int main(void) {
    Bdisp_AllClr_VRAM();

    SetQuitHandler(quithandler);

    //optional overclock if needed
    //change_freq(PLL_18x);

    int keybinds[12] = {KEY_CTRL_OPTN, KEY_CHAR_SQUARE, KEY_CTRL_F6, KEY_CTRL_F6, KEY_CTRL_UP, KEY_CTRL_DOWN, KEY_CTRL_LEFT, KEY_CTRL_RIGHT, KEY_CTRL_SHIFT, KEY_CTRL_ALPHA, KEY_CTRL_F1, KEY_CTRL_F2};//{B, Y, Select, Start, UP, DOWN, LEFT, RIGHT, A, X, L, R}

    Rom rom = main_menu_ui(keybinds);
    init_cpu(rom);//rom gets loaded here

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
