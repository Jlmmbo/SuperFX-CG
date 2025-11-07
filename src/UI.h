//UI functions here

//function prototypes

int map_key_ui(char*);
Rom main_menu_ui(int[12]);



int map_key_ui(char* key_name){
    int key = 0;
    Bdisp_Rectangle(20, 30, 364, 162, TEXT_COLOR_WHITE);//find better dimensions
    display_text();
    GetKey(&key);
    return key;
}

Rom main_menu_ui(int keybinds[12]){
    while (1){
        keyupdate();
        if (keydownlast(KEY_SHIFT_OPTN)){
            //Settings
        }
        if (keydownlast(KEY_CTRL_OPTN)){
            keybinds[0] = map_key_ui("D-UP");
            keybinds[1] = map_key_ui("D-DOWN");
            keybinds[2] = map_key_ui("D-LEFT");
            keybinds[3] = map_key_ui("D-RIGHT");
            keybinds[4] = map_key_ui("A");
            keybinds[5] = map_key_ui("B");
            keybinds[6] = map_key_ui("X");
            keybinds[7] = map_key_ui("Y");
            keybinds[8] = map_key_ui("L");
            keybinds[9] = map_key_ui("R");
            keybinds[10] = map_key_ui("START");
            keybinds[11] = map_key_ui("SELECT");
        }
    }
    return test_rom;
}