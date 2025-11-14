//UI functions here

//function prototypes

int map_key_ui(char*);
Rom main_menu_ui(int[12]);

void put_disp(void);
void put_disp_strip(unsigned, unsigned);

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
      DmaWaitNext();//make sure no ongoing transfer is already happening
      Bdisp_WriteDDRegister3_bit7(1);
      Bdisp_DefineDMARange(6,389,y1,y2);
      Bdisp_DDRegisterSelect(LCD_GRAM);

      *MSTPCR0&=~(1<<21);//Clear bit 21
      *DMA0_CHCR_0&=~1;//Disable DMA on channel 0
      *DMA0_DMAOR=0;//Disable all DMA
      *DMA0_SAR_0=((unsigned int)GetVRAMAddress()+(y1*384*2))&0x1FFFFFFF;//Source address is VRAM
      *DMA0_DAR_0=LCD_BASE&0x1FFFFFFF;//Destination is LCD
      *DMA0_TCR_0=((y2-y1+1)*384)/16;//Transfer count bytes/32
      *DMA0_CHCR_0=0x00101400;
      *DMA0_DMAOR|=1;//Enable DMA on all channels
      *DMA0_DMAOR&=~6;//Clear flags
      *DMA0_CHCR_0|=1;//Enable channel0 DMA
   }

   void put_disp(void){
      put_disp_strip(0,216);
   }
#else
   void put_disp(void){
      Bdisp_PutDisp_DD();
   }

   void put_disp_strip(unsigned y1, unsigned y2){
      Bdisp_PutDisp_DD_stripe(y1, y2);
   }
#endif

/// @brief display text of any reasonable size
/// @param color TEXT_COLOR defined in display.h
/// @param font_size size of the font to be displayed in px
/// @param x The x coordinate (in pixels) of the upper left corner of the first character
/// @param y The y coordinate (in pixels) of the upper left corner of the first character
void display_text(char* text, char color, unsigned char font_size, int x, int y){
    PrintCXY(x, y, text, TEXT_MODE_NORMAL, -1, TEXT_COLOR_BLACK, TEXT_COLOR_WHITE, 1, 0);
}

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

void CopySpriteNbitMasked(const unsigned char* data, int x, int y, int width, int height, const color_t* palette, color_t maskColor, unsigned int bitwidth)  
 {
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
        while(w2--)
            *s++ = col;
        s += 384 - w;
    }
}

int map_key_ui(char* key_name){
    int key = 0;
    Bdisp_Rectangle(20, 30, 364, 162, TEXT_COLOR_WHITE);//find better dimensions
    display_text(key_name, TEXT_COLOR_BLACK, 15, 30, 40);
    GetKey(&key);
    return key;
}

Rom main_menu_ui(int keybinds[12]){
    while (1){
        keyupdate();
        put_disp();
        PrintXY(1, 2, "  Main menu", 0, 0);
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
        if (keydownlast(KEY_CTRL_EXE)){
            return test_rom;
        }
    }
    return test_rom;
}

void pause_menu_ui(CPUState* cpu){
    while (1){
        keyupdate();
        put_disp();

        PrintXY(1, 1, "  Paused", 0, TEXT_COLOR_BLACK);

        if(keydownlast(KEY_CTRL_EXIT)){
            break;
        }
    }
}