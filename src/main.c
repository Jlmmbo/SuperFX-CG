#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/heap.h>
#include <fxcg/rtc.h>

#include "main.h"
 
int main(void) {
    Bdisp_AllClr_VRAM();

    //char menu_screen_index = 0;//0: main menu, 1: in-game
    int keybinds[12] = {KEY_CTRL_UP, KEY_CTRL_DOWN, KEY_CTRL_LEFT, KEY_CTRL_RIGHT, KEY_CTRL_SHIFT, KEY_CTRL_OPTN, KEY_CTRL_ALPHA, KEY_CHAR_SQUARE, KEY_CTRL_F1, KEY_CTRL_F2, KEY_CTRL_F5, KEY_CTRL_F6};//{D-UP, D-DOWN, D-LEFT, D-RIGHT, A, B, X, Y, L, R, START, SELECT}

    CPUState cpu;
    Rom rom = main_menu_ui(keybinds);
    init_cpu(&cpu, rom);//rom gets loaded here
    while (1){
        /*
        //menu loop
        while (1) {
            keyupdate();
            if (keydownlast(KEY_CTRL_MENU)) return 1;
        }
        */
        //game loop
        while (1) {
            keyupdate();
            if (keydownlast(KEY_CTRL_MENU)) break;

            cycle_cpu(&cpu);
        }
        pause_menu_ui(&cpu);
    }
    return 0;
}
