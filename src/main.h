#include <string.h>
#include <stdint.h>

#include "overclock.h"

#define NON_BLOCK_DMA 1
#define true (1)
#define false (0)

#define INIDISP (cpu->mem[0x00][0x2100])
#define OBJSEL (cpu->mem[0x00][0x2101])
#define OAMADDL (cpu->mem[0x00][0x2102])
#define OAMADDH (cpu->mem[0x00][0x2103])
#define OAMDATA (cpu->mem[0x00][0x2104])
#define BGMODE (cpu->mem[0x00][0x2105])
#define MOSAIC (cpu->mem[0x00][0x2106])
#define BG1SC (cpu->mem[0x00][0x2107])
#define BG2SC (cpu->mem[0x00][0x2108])
#define BG3SC (cpu->mem[0x00][0x2109])
#define BG4SC (cpu->mem[0x00][0x210a])
#define BG12NBA (cpu->mem[0x00][0x210b])
#define BG34NBA (cpu->mem[0x00][0x210c])
#define BG1HOFS (cpu->mem[0x00][0x210d])
#define BG1VOFS (cpu->mem[0x00][0x210e])
#define BG2HOFS (cpu->mem[0x00][0x210f])
#define BG2VOFS (cpu->mem[0x00][0x2110])
#define BG3HOFS (cpu->mem[0x00][0x2111])
#define BG3VOFS (cpu->mem[0x00][0x2112])
#define BG4HOFS (cpu->mem[0x00][0x2113])
#define BG4VOFS (cpu->mem[0x00][0x2114])
#define VMAIN (cpu->mem[0x00][0x2115])
#define VMADDL (cpu->mem[0x00][0x2116])
#define VMADDH (cpu->mem[0x00][0x2117])
#define VMDATAL (cpu->mem[0x00][0x2118])
#define VMDATAH (cpu->mem[0x00][0x2119])
#define M7SEL (cpu->mem[0x00][0x211a])
#define M7A (cpu->mem[0x00][0x211b])
#define M7B (cpu->mem[0x00][0x211c])
#define M7C (cpu->mem[0x00][0x211d])
#define M7D (cpu->mem[0x00][0x211e])
#define M7X (cpu->mem[0x00][0x211f])
#define M7Y (cpu->mem[0x00][0x2120])
#define CGADD (cpu->mem[0x00][0x2121])
#define CGDATA (cpu->mem[0x00][0x2122])
#define W12SEL (cpu->mem[0x00][0x2123])
#define W34SEL (cpu->mem[0x00][0x2124])
#define WOBJSEL (cpu->mem[0x00][0x2125])
#define WH0 (cpu->mem[0x00][0x2126])
#define WH1 (cpu->mem[0x00][0x2127])
#define WH2 (cpu->mem[0x00][0x2128])
#define WH3 (cpu->mem[0x00][0x2129])
#define WBGLOG (cpu->mem[0x00][0x212a])
#define WOBJLOG (cpu->mem[0x00][0x212b])
#define TM (cpu->mem[0x00][0x212c])
#define TS (cpu->mem[0x00][0x212d])
#define TMW (cpu->mem[0x00][0x212e])
#define TSW (cpu->mem[0x00][0x212f])
#define CGWSEL (cpu->mem[0x00][0x2130])
#define CGADSUB (cpu->mem[0x00][0x2131])
#define COLDATA (cpu->mem[0x00][0x2132])
#define SETINI (cpu->mem[0x00][0x2133])
#define MPYL (cpu->mem[0x00][0x2134])
#define MPYM (cpu->mem[0x00][0x2135])
#define MPYH (cpu->mem[0x00][0x2136])
#define SLHV (cpu->mem[0x00][0x2137])
#define OAMDATAREAD (cpu->mem[0x00][0x2138])
#define VMDATALREAD (cpu->mem[0x00][0x2139])
#define VMDATAHREAD (cpu->mem[0x00][0x213a])
#define CGDATAREAD (cpu->mem[0x00][0x213b])
#define OPHCT (cpu->mem[0x00][0x213c])
#define OPVCT (cpu->mem[0x00][0x213d])
#define STAT77 (cpu->mem[0x00][0x213e])
#define STAT78 (cpu->mem[0x00][0x213f])

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
    uint16_t* VRAM;
}PPU;

typedef struct CPUState{
    byte NMI:1;
    byte IRQ:1;
    byte RES:1;
    byte E:1;
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

void quithandler(){
    change_freq(PLL_16x);
}

#include "binary.h"
#include "65C816.h"
#include "ppu.h"
#include "filesystem.h"
#include "UI.h"