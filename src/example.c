#include <fxcg/display.h>
#include <fxcg/keyboard.h>
 
void main(void) {
    int key;
     
    Bdisp_AllClr_VRAM();
    PrintXY(1, 1, "Press EXE to exit", 0, 0);

    while (1) {
        GetKey(&key);

        if (key == KEY_CTRL_EXE) {
            break;
        }
    }
 
    return;
}
