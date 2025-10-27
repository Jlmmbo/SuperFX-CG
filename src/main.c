#include <fxcg/display.h>
#include <fxcg/keyboard.h>
#include <fxcg/heap.h>

#include "main.h"
#include "binary.h"
#include "65C816.h"
 
int main(void) {
    Bdisp_AllClr_VRAM();

    CPUState cpu;
    init_cpu(&cpu);

    while (1) {
        keyupdate();
        if (keydownlast(KEY_CTRL_MENU)) return 0;
        cycle_cpu(&cpu);
    }

    return 0;
}
