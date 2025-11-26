#include <string.h>
#include <stdint.h>

#define NON_BLOCK_DMA 1
#define true (1)
#define false (0)

void put_disp(void);

const unsigned short* keyboard_register = (unsigned short*)0xA44B0000;
unsigned short lastkey[8];
unsigned short holdkey[8];

void keyupdate(void) {
   memcpy(holdkey, lastkey, sizeof(unsigned short)*8);
   memcpy(lastkey, keyboard_register, sizeof(unsigned short)*8);
}

int keydownlast(int basic_keycode) {
   int row, col, word, bit; 
   row = basic_keycode%10; 
   col = basic_keycode/10-1; 
   word = row>>1; 
   bit = col + 8*(row&1); 
   return (0 != (lastkey[word] & 1<<bit)); 
}

int keydownhold(int basic_keycode) {
   int row, col, word, bit; 
   row = basic_keycode%10; 
   col = basic_keycode/10-1; 
   word = row>>1; 
   bit = col + 8*(row&1); 
   return (0 != (holdkey[word] & 1<<bit)); 
}

typedef unsigned char byte;

typedef struct{
    byte h:8;//high byte
    byte l:8;//low byte
}two_bytes;

typedef struct address{
    byte bank;
    uint16_t addr;
}address;

typedef struct StatusReg{
    char N:1;//bit 7: Negative
    char V:1;//bit 6: oVerflow
    char M:1;//bit 5: Memory/accumulator size (0: 16-bit, 1: 8-bit)
    char X:1;//bit 4: indeX size (0:16-bit, 1:8-bit)
    char D:1;//bit 3: Decimal mode
    char I:1;//bit 2: Interrupt disable (only on IRQ)
    char Z:1;//bit 1: Zero
    char E:1;//bit 0: Emulation or Carry
}StatusReg;

typedef struct OBJ{
    unsigned short x_start;
    unsigned short x_end;
    unsigned short y_statrt;
    unsigned short y_end;
}OBJ;

typedef struct Tile{
   byte data[8][8];
}Tile;

typedef struct PPU{
    unsigned short h_cntr;//maybe fit into a byte
    unsigned short v_cntr;//maybe fit into a byte
    //registers
    byte BG1SC;
    byte BG2SC;
    byte BG3SC;
    byte BG4SC;
    byte BG1HOFS;
    byte BG2HOFS;
    byte BG3HOFS;
    byte BG4HOFS;
    byte BG1VOFS;
    byte BG2VOFS;
    byte BG3VOFS;
    byte BG4VOFS;
    byte BG12NBA;
    byte BG34NBA;
    byte BGCOLOR;
    byte CGADD;
    uint16_t VRAM[32768];
}PPU;

typedef struct CPUState{
    byte NMI:1;
    byte IRQ:1;
    byte RES:1;
    uint16_t X, Y;//index registers
    uint16_t D;//direct register
    two_bytes C;//accumulator
    StatusReg P;//processor status register
    uint16_t PC;//program counter
    two_bytes S;//stack address register
    byte DBR;//data bank register
    byte PBR;//program bank register
    byte* mem[256];//mem bank pointers
    unsigned int expected_time;//when the current instruction should finish
    unsigned int current_time;
    PPU ppu;
    unsigned int rom_mode;
}CPUState;

typedef struct Rom{
    byte* raw;
    long int size:24;//long to ensure at least 24 bits reg. int may only be 16
}Rom;

typedef struct instr_data{
    address (*addr_func)(CPUState*);//function to calculate effective address
    byte cycle_count;
    byte read_bytes;//num of bytes read, includes opcode
    void (*instr_func)(CPUState*, address);//function to excecute instruction
}instr_data;

#include "binary.h"
#include "65C816.h"
#include "ppu.h"
#include "filesystem.h"
#include "UI.h"