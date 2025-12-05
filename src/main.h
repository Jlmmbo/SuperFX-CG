#include <string.h>
#include <stdint.h>

#include "overclock.h"

//#define NON_BLOCK_DMA 1
#define true (1)
#define false (0)

#define INIDISP (*mem_ptr(cpu, 0x2100))
#define OBJSEL (*mem_ptr(cpu, 0x2101))
#define OAMADDL (*mem_ptr(cpu, 0x2102))
#define OAMADDH (*mem_ptr(cpu, 0x2103))
#define OAMDATA (*mem_ptr(cpu, 0x2104))
#define BGMODE (*mem_ptr(cpu, 0x2105))
#define MOSAIC (*mem_ptr(cpu, 0x2106))
#define BG1SC (*mem_ptr(cpu, 0x2107))
#define BG2SC (*mem_ptr(cpu, 0x2108))
#define BG3SC (*mem_ptr(cpu, 0x2109))
#define BG4SC (*mem_ptr(cpu, 0x210a))
#define BG12NBA (*mem_ptr(cpu, 0x210b))
#define BG34NBA (*mem_ptr(cpu, 0x210c))
#define BG1HOFS (*mem_ptr(cpu, 0x210d))
#define BG1VOFS (*mem_ptr(cpu, 0x210e))
#define BG2HOFS (*mem_ptr(cpu, 0x210f))
#define BG2VOFS (*mem_ptr(cpu, 0x2110))
#define BG3HOFS (*mem_ptr(cpu, 0x2111))
#define BG3VOFS (*mem_ptr(cpu, 0x2112))
#define BG4HOFS (*mem_ptr(cpu, 0x2113))
#define BG4VOFS (*mem_ptr(cpu, 0x2114))
#define VMAIN (*mem_ptr(cpu, 0x2115))
#define VMADDL (*mem_ptr(cpu, 0x2116))
#define VMADDH (*mem_ptr(cpu, 0x2117))
#define VMDATAL (*mem_ptr(cpu, 0x2118))
#define VMDATAH (*mem_ptr(cpu, 0x2119))
#define M7SEL (*mem_ptr(cpu, 0x211a))
#define M7A (*mem_ptr(cpu, 0x211b))
#define M7B (*mem_ptr(cpu, 0x211c))
#define M7C (*mem_ptr(cpu, 0x211d))
#define M7D (*mem_ptr(cpu, 0x211e))
#define M7X (*mem_ptr(cpu, 0x211f))
#define M7Y (*mem_ptr(cpu, 0x2120))
#define CGADD (*mem_ptr(cpu, 0x2121))
#define CGDATA (*mem_ptr(cpu, 0x2122))
#define W12SEL (*mem_ptr(cpu, 0x2123))
#define W34SEL (*mem_ptr(cpu, 0x2124))
#define WOBJSEL (*mem_ptr(cpu, 0x2125))
#define WH0 (*mem_ptr(cpu, 0x2126))
#define WH1 (*mem_ptr(cpu, 0x2127))
#define WH2 (*mem_ptr(cpu, 0x2128))
#define WH3 (*mem_ptr(cpu, 0x2129))
#define WBGLOG (*mem_ptr(cpu, 0x212a))
#define WOBJLOG (*mem_ptr(cpu, 0x212b))
#define TM (*mem_ptr(cpu, 0x212c))
#define TS (*mem_ptr(cpu, 0x212d))
#define TMW (*mem_ptr(cpu, 0x212e))
#define TSW (*mem_ptr(cpu, 0x212f))
#define CGWSEL (*mem_ptr(cpu, 0x2130))
#define CGADSUB (*mem_ptr(cpu, 0x2131))
#define COLDATA (*mem_ptr(cpu, 0x2132))
#define SETINI (*mem_ptr(cpu, 0x2133))
#define MPYL (*mem_ptr(cpu, 0x2134))
#define MPYM (*mem_ptr(cpu, 0x2135))
#define MPYH (*mem_ptr(cpu, 0x2136))
#define SLHV (*mem_ptr(cpu, 0x2137))
#define OAMDATAREAD (*mem_ptr(cpu, 0x2138))
#define VMDATALREAD (*mem_ptr(cpu, 0x2139))
#define VMDATAHREAD (*mem_ptr(cpu, 0x213a))
#define CGDATAREAD (*mem_ptr(cpu, 0x213b))
#define OPHCT (*mem_ptr(cpu, 0x213c))
#define OPVCT (*mem_ptr(cpu, 0x213d))
#define STAT77 (*mem_ptr(cpu, 0x213e))
#define STAT78 (*mem_ptr(cpu, 0x213f))

void put_disp(void);
void pause_menu_ui();
void error_msg(char*);

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
  unsigned short id, type;
  unsigned long fsize, dsize;
  unsigned int property;
  unsigned long address;
} file_type_t;

typedef struct{
    byte h:8;//high byte
    byte l:8;//low byte
}two_bytes;

/*typedef struct address{
    byte bank;
    uint16_t addr;
}address;*/
typedef uint32_t address;

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
    PPU ppu;
    unsigned int rom_mode;
}CPUState;

typedef struct Rom{
    byte* raw;
    long int size:24;//long to ensure at least 24 bits reg. int may only be 16
}Rom;

typedef struct instr_data{
    address (*addr_func)(CPUState*);//function to calculate effective address
    byte read_bytes;//num of bytes read, includes opcode
    void (*instr_func)(CPUState*, address);//function to excecute instruction
}instr_data;

void quithandler(){
    change_freq(PLL_16x);
}

#include "binary.h"
#include "65C816.h"
#include "dma.h"
#include "ppu.h"
#include "filesystem.h"
#include "UI.h"