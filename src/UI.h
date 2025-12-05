//UI functions here

//function prototypes

#ifdef NON_BLOCK_DMA
   #define LCD_GRAM    0x202
   #define LCD_BASE    0xB4000000
   #define SYNCO() __asm__ volatile("SYNCO\n\t":::"memory");
   // Module Stop Register 0
   #define MSTPCR0     (volatile unsigned*)0xA4150030
   // DMA0 operation register
   #define DMA0_DMAOR  (volatile unsigned short*)0xFE008060
   #define DMA0_SAR_0  (volatile unsigned*)0xFE008020
   #define DMA0_DAR_0  (volatile unsigned*)0xFE008024
   #define DMA0_TCR_0  (volatile unsigned*)0xFE008028
   #define DMA0_CHCR_0 (volatile unsigned*)0xFE00802C
   void DmaWaitNext(void){
      while(1){
         if((*DMA0_DMAOR)&4)//Address error has occurred stop looping
               break;
         if((*DMA0_CHCR_0)&2)//Transfer is done
               break;
      }
      SYNCO();
      *DMA0_CHCR_0&=~1;
      *DMA0_DMAOR=0;
   }
   void put_disp_strip(unsigned y1,unsigned y2){
      DmaWaitNext();
      Bdisp_WriteDDRegister3_bit7(1);
      Bdisp_DefineDMARange(6,389,y1,y2);
      Bdisp_DDRegisterSelect(LCD_GRAM);

      *MSTPCR0&=~(1<<21);//Clear bit 21
      *DMA0_CHCR_0&=~1;//Disable DMA on channel 0
      *DMA0_DMAOR=0;//Disable all DMA
      *DMA0_SAR_0=((int)(GetVRAMAddress()+(y1 * 384 * 2)) & 0x1FFFFFFF);//Source address is VRAM
      *DMA0_DAR_0=LCD_BASE&0x1FFFFFFF;//Destination is LCD
      *DMA0_TCR_0=((y2-y1+1)*384)/16;//Transfer count bytes/32
      *DMA0_CHCR_0=0x00101400;
      *DMA0_DMAOR|=1;//Enable DMA on all channels
      *DMA0_DMAOR&=~6;//Clear flags
      *DMA0_CHCR_0|=1;//Enable channel0 DMA
   }

   void put_disp(void){
      put_disp_strip(0,215);
   }
#else
   void put_disp(void){
      Bdisp_PutDisp_DD();
   }

   void put_disp_strip(unsigned y1, unsigned y2){
      Bdisp_PutDisp_DD_stripe(y1, y2);
   }
#endif

void CopySprite(color_t* sprite, int x, int y, int width, int height) {
   color_t* VRAM = (color_t*)GetVRAMAddress();
   VRAM += LCD_WIDTH_PX*y + x;
   for(int j=y; j<y+height; j++) {
      for(int i=x; i<x+width; i++) {
         *(VRAM++) = *(sprite++);
      }
      VRAM += LCD_WIDTH_PX-width;
   }
} 

void CopySpriteMaskedAlpha(const void*datar, int x, int y, int width, int height, color_t maskcolor, int alpha) { 
   color_t*data = (color_t*) datar; 
   color_t* VRAM = (color_t*)GetVRAMAddress(); 
   VRAM += LCD_WIDTH_PX*y + x; 
   alpha %= 32; 
   for(int j=y; j<y+height; j++) { 
      for(int i=x; i<x+width;  i++) { 
         if (*(data) != maskcolor) { 
         *(VRAM) = (color_t)((((int)(*data & 0xf81f) * alpha + (int)(*VRAM & 0xf81f) * (32-alpha) + 0x8010) >> 5) & 0xf81f) | 
                (color_t)((((int)(*data & 0x07e0) * alpha + (int)(*VRAM & 0x07e0) * (32-alpha) + 0x0400) >> 6) & 0x07e0); 
           VRAM++; data++; 
         } else { VRAM++; data++; } 
      }
      VRAM += LCD_WIDTH_PX-width; 
   } 
}

void CopySpriteNbit(const unsigned char* data, int x, int y, int width, int height, const color_t* palette, unsigned int bitwidth) {
   color_t* VRAM = (color_t*) GetVRAMAddress();
   VRAM += (LCD_WIDTH_PX*y + x);
   int offset = 0;
   unsigned char buf;
   for(int j=y; j<y+height; j++) {
      int availbits = 0;
      for(int i=x; i<x+width;  i++) {
         if (!availbits) {
            buf = data[offset++];
            availbits = 8;
         }
         color_t this = ((color_t)buf>>(8-bitwidth));
         *VRAM = palette[(color_t)this];
         VRAM++;
         buf<<=bitwidth;
         availbits-=bitwidth;
      }
      VRAM += (LCD_WIDTH_PX-width);
   }
}

void CopySpriteNbitMasked(const unsigned char* data, int x, int y, int width, int height, const color_t* palette, color_t maskColor, unsigned int bitwidth) {
   color_t* VRAM = (color_t*) GetVRAMAddress();
   VRAM += (LCD_WIDTH_PX * y + x);

   int offset = 0;

   unsigned char buf;

   for(int j=y; j<y+height; j++) {

      int availbits = 0;

      for(int i=x; i<x+width;  i++) {

         if (!availbits) {
            buf = data[offset++];
            availbits = 8;
         }

         color_t color = palette[(color_t)buf>>(8-bitwidth)];

         if(color != maskColor) {
            *VRAM = color;
         }
         VRAM++;
         buf<<=bitwidth;
         availbits-=bitwidth;
      }
      VRAM += (LCD_WIDTH_PX-width);
   }
}

void fillArea(unsigned x, unsigned y, unsigned w, unsigned h, unsigned short col){
    unsigned short *s = (unsigned short *)GetVRAMAddress();
    s += (y * 384) + x;
    while(h--){
        unsigned w2 = w;
        while(w2--) *s++ = col;
        s += 384 - w;
    }
}

int map_key_ui(char* key_name){
    int key = 0;
    Bdisp_AllClr_VRAM();
    Bdisp_Rectangle(20, 30, 364, 162, TEXT_COLOR_WHITE);//find better dimensions
    //display_text(key_name, TEXT_COLOR_BLACK, 15, 30, 40);
    PrintXY(1, 1, key_name, 0, TEXT_COLOR_BLUE);
    GetKey(&key);
    return key;
}

unsigned char list_menu_ui(char* title, char* options[], unsigned char option_count){
   unsigned char list_index = 0;
   int k;
   Bdisp_AllClr_VRAM();
   while (1){
      PrintCXY(1, 1, title, 0, -1, COLOR_BLUE, COLOR_WHITE, 1, 0);
      for (unsigned char i = 0; i < option_count; i++){
         if (i == list_index){
            PrintCXY(22, (3 + i) * 20, options[i], 0, -1, COLOR_BLACK, COLOR_WHITE, 1, 0);
            PrintCXY(1, (3 + i) * 20, ">", 0, -1, COLOR_BLACK, COLOR_WHITE, 1, 0);
         } else {
            PrintCXY(22, (3 + i) * 20, options[i], 0, -1, COLOR_BLUE, COLOR_WHITE, 1, 0);
            PrintCXY(1, (3 + i) * 20, " ", 0, -1, COLOR_BLUE, COLOR_WHITE, 1, 0);
         }
      }
      GetKey(&k);
      if (k == KEY_CTRL_UP){
         if (list_index == 0) list_index = option_count - 1;
         else list_index--;
      } else if (k == KEY_CTRL_DOWN){
         list_index++;
         if (list_index >= option_count) list_index = 0;
      } else if (k == KEY_CTRL_EXE){
         return list_index;
      }
   }
   return list_index;
}

void settings_menu_ui(){
   //settings
   unsigned char option = list_menu_ui("Settings", (char*[]){"Keybinds", "Brightness", "Frameskip", "Colour Palette", "Overclock"}, 4);
   switch(option){
      case 0://keybinds
         keybinds[4] = map_key_ui("  D-UP");
         keybinds[5] = map_key_ui("  D-DOWN");
         keybinds[6] = map_key_ui("  D-LEFT");
         keybinds[7] = map_key_ui("  D-RIGHT");
         keybinds[8] = map_key_ui("  A");
         keybinds[0] = map_key_ui("  B");
         keybinds[9] = map_key_ui("  X");
         keybinds[1] = map_key_ui("  Y");
         keybinds[10] = map_key_ui("  L");
         keybinds[11] = map_key_ui("  R");
         keybinds[3] = map_key_ui("  START");
         keybinds[2] = map_key_ui("  SELECT");
         break;
      case 1://brightness
         list_menu_ui("Brightness", (char*[]){"Ultra low", "Low", "Medium", "High", "Very high"}, 5);
         break;
      case 2://frameskip
         break;
      case 3://colour palette
         break;
      case 4://Overclock
         break;
   }
}


Rom main_menu_ui(){
   while (1){
      unsigned char menu_index = list_menu_ui("Main menu", (char*[]){"Settings", "Load ROM", "Exit"}, 3);
      switch (menu_index) {
         case 0:{
            settings_menu_ui();
            break;
         }
         case 1:{
            //load ROM
            char* rom_selection_list[] = {sys_malloc(sizeof(char) * 16), sys_malloc(sizeof(char) * 16), sys_malloc(sizeof(char) * 16), sys_malloc(sizeof(char) * 16),sys_malloc(sizeof(char) * 16), sys_malloc(sizeof(char) * 16), sys_malloc(sizeof(char) * 16), sys_malloc(sizeof(char) * 16)};
            byte rom_list_len;
            get_rom_list_fs(rom_selection_list, &rom_list_len);
            if(rom_selection_list == NULL || rom_list_len == 0){
               Bdisp_AllClr_VRAM();
               error_msg("  No ROMs");
            }
            Rom selected_rom = load_rom_fs(rom_selection_list, list_menu_ui("SELECT ROM", rom_selection_list, rom_list_len));
            return selected_rom;
         }
         case 2:{
            Bdisp_AllClr_VRAM();
            PrintXY(1, 1, "  Press MENU to exit", TEXT_MODE_NORMAL, TEXT_COLOR_BLACK);
            int k;
            GetKey(&k);
            break;
         }
      }
   }
   return test_rom;
}

void pause_menu_ui(){
   int k;
   Bdisp_AllClr_VRAM();
   PrintXY(1, 1, "  Paused", 0, TEXT_COLOR_BLACK);
   GetKey(&k);
}

void error_msg(char* msg){
   Bdisp_AllClr_VRAM();
   PrintCXY(1, 1, msg, 0, -1, 0, COLOR_WHITE, 1, 0);
   GetKey(NULL);
}

//not done

// void pause_menu_ui(CPUState* cpu){
// -    while (1){
// -        keyupdate();
// -        put_disp();
// -        PrintXY(1, 1, "  Paused", 0, TEXT_COLOR_BLACK);
// -
// -        if(keydownlast(KEY_CTRL_EXIT)){
// -            break;
// -        }
// -    }
// +    // Simple pause menu with common actions
// +    const char* options[] = {"  Resume", "  Save State", "  Load State", "  Restart ROM", "  Exit to Menu"};
// +    while (1){
// +        keyupdate();
// +        put_disp();
// +        PrintXY(1, 1, "  Paused", 0, TEXT_COLOR_WHITE);
// +        unsigned char sel = list_menu_ui("  Paused", (char**)options, 5);
// +        switch(sel){
// +            case 0: // Resume
// +                return;
// +            case 1: // Save State (ask for slot or use quick slot 0)
// +                // implement save_state(slot) in your emulator core
// +                save_state(0);
// +                // show brief confirmation (implement show_toast)
// +                show_toast("State saved");
// +                break;
// +            case 2: // Load State
// +                load_state(0);
// +                show_toast("State loaded");
// +                break;
// +            case 3: // Restart ROM
// +                restart_rom();
// +                return;
// +            case 4: // Exit to menu
// +                exit_to_menu();
// +                return;
// +        }
// +    }
//  }
//  // ...existing code...
 
//  // Placeholder prototypes - implement these in your core
//  void save_state(int slot);
//  void load_state(int slot);
//  void restart_rom(void);
//  void exit_to_menu(void);
//  void show_toast(const char* text);