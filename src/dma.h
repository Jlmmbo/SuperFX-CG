#define MDMAEN (*mem_ptr(cpu, 0x420B))
#define HDMAEN (*mem_ptr(cpu, 0x420C))
#define DMAP(n) (*mem_ptr(cpu, 0x4300 + ((n) << 4)))
#define BBADn(N) (*mem_ptr(cpu, 0x4301 + ((n) << 4)))
#define A1TnL(N) (*mem_ptr(cpu, 0x4302 + ((n) << 4)))
#define A1TnH(N) (*mem_ptr(cpu, 0x4303 + ((n) << 4)))
#define A1Bn(N) (*mem_ptr(cpu, 0x4304 + ((n) << 4)))

#define DASnL(N) (*mem_ptr(cpu, 0x4305 + ((n) << 4)))
#define DASnH(N) (*mem_ptr(cpu, 0x4306 + ((n) << 4)))
#define DASBn(N) (*mem_ptr(cpu, 0x4307 + ((n) << 4)))

#define A2TnL(N) (*mem_ptr(cpu, 0x4308 + ((n) << 4)))

#define A2TnH(N) (*mem_ptr(cpu, 0x4309 + ((n) << 4)))

#define NLTRn(N) (*mem_ptr(cpu, 0x430A + ((n) << 4)))

//#define UNUSEDn(N) (*mem_ptr(cpu, 0x4300 + ((n) << 4)))

void updateDMA(CPUState* cpu){
    byte mdmaen = MDMAEN;
    if (mdmaen & 0b10000000){
        ;//dma channel 7
    }
    if (mdmaen & 0b01000000){
        ;//dma channel 6
    }
    if (mdmaen & 0b00100000){
        ;//dma channel 5
    }
    if (mdmaen & 0b00010000){
        ;//dma channel 4
    }
    if (mdmaen & 0b00001000){
        ;//dma channel 3
    }
    if (mdmaen & 0b00000100){
        ;//dma channel 2
    }
    if (mdmaen & 0b00000010){
        ;//dma channel 1
    }
    if (mdmaen & 0b00000001){
        ;//dma channel 0
    }
}