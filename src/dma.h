#define MDMAEN (*mem_ptr(0x420B))
#define HDMAEN (*mem_ptr(0x420C))
#define DMAP(N) (*mem_ptr(0x4300 + ((N) << 4)))
#define BBADn(N) (*mem_ptr(0x4301 + ((N) << 4)))
#define A1TnL(N) (*mem_ptr(0x4302 + ((N) << 4)))
#define A1TnH(N) (*mem_ptr(0x4303 + ((N) << 4)))
#define A1Bn(N) (*mem_ptr(0x4304 + ((N) << 4)))
#define DASnL(N) (*mem_ptr(0x4305 + ((N) << 4)))
#define DASnH(N) (*mem_ptr(0x4306 + ((N) << 4)))
#define DASBn(N) (*mem_ptr(0x4307 + ((N) << 4)))
#define A2AnL(N) (*mem_ptr(0x4308 + ((N) << 4)))
#define A2AnH(N) (*mem_ptr(0x4309 + ((N) << 4)))
#define NLTRn(N) (*mem_ptr(0x430A + ((N) << 4)))

const byte hdma_num_bytes_trans[8] = {1, 2, 2, 4, 4, 4, 2, 4};
const byte hdma_trans_patterns[8][4] = {{0,0,0,0},{0,1,0,0},{0,0,0,0},{0,0,1,1},{0,1,2,3},{0,1,0,1},{0,0,0,0},{0,0,1,1}};

void doDMA(byte channel, byte hdma){
    byte dmap = DMAP(channel);
    byte val;
    address addr;
    if(hdma){
        byte i;
        if(!(dmap & 0b01000000)){//hdma indirect
            byte trans_pattern = dmap & 0b00000111;
            //A2AnL(channel) = A1TnL(channel);
            //A2AnH(channel) = A1TnH(channel);
            val = mem_fetch((A1Bn(channel)) + (A2AnH(channel) << 8) + A2AnL(channel));
            for(i = 0; i <= hdma_num_bytes_trans[trans_pattern]; i++){
                mem_set(val, BBADn(channel) + hdma_trans_patterns[trans_pattern][i]);
            }
        } else {
            val = mem_fetch((A1Bn(channel) << 16) + (A1TnH(channel) << 8) + A1TnL(channel));

            byte trans_pattern = dmap & 0b00000111;
            for(i = 0; i <= hdma_num_bytes_trans[trans_pattern]; i++){
                mem_set(val, BBADn(channel) + hdma_trans_patterns[trans_pattern][i]);
            }
        }
    } else {
        if (dmap & 0b10000000){//copier de b a a
            val = mem_fetch(BBADn(channel));//a faire: clamp a 2100-21ff
            addr = (A1Bn(channel) << 16) + (A1TnH(channel) << 8) + A1TnL(channel);
        } else {
            val = mem_fetch((A1Bn(channel) << 16) + (A1TnH(channel) << 8) + A1TnL(channel));
            addr = BBADn(channel);//a faire: clamp a 2100-21ff
        }
        while((DASnH(channel) != 0) && (DASnL(channel) != 0)){
            mem_set(val, addr);

            if(!(dmap & 0b00001000)){//pas d'addresse fixe
                addr -= ((dmap & 0b00010000) >> 3) - 1;//si 0: addr -= -1 sinon: addr -= (2) - 1
                //a faire: clamp a 2100-21ff
            }

            DASnH(channel) -= DASnL(channel) == 0;
            DASnL(channel)--;
        }
    }
}

void updateDMA(){
    byte mdmaen = MDMAEN;
    for(byte i = 0; i < 8; i++){
        A2AnH(i) = A1TnH(i);
        A2AnL(i) = A1TnL(i);
    }
    if (mdmaen & 0b10000000){
        doDMA(7, 0);
    }
    if (mdmaen & 0b01000000){
        doDMA(6, 0);
    }
    if (mdmaen & 0b00100000){
        doDMA(5, 0);
    }
    if (mdmaen & 0b00010000){
        doDMA(4, 0);
    }
    if (mdmaen & 0b00001000){
        doDMA(3, 0);
    }
    if (mdmaen & 0b00000100){
        doDMA(2, 0);
    }
    if (mdmaen & 0b00000010){
        doDMA(1, 0);
    }
    if (mdmaen & 0b00000001){
        doDMA(0, 0);
    }
    byte hdmaen = MDMAEN;
    if (hdmaen & 0b10000000){
        doDMA(7, 1);//hdma channel 7
    }
    if (hdmaen & 0b01000000){
        doDMA(6, 1);//hdma channel 6
    }
    if (hdmaen & 0b00100000){
        doDMA(5, 1);//hdma channel 5
    }
    if (hdmaen & 0b00010000){
        doDMA(4, 1);//hdma channel 4
    }
    if (hdmaen & 0b00001000){
        doDMA(3, 1);//hdma channel 3
    }
    if (hdmaen & 0b00000100){
        doDMA(2, 1);//hdma channel 2
    }
    if (hdmaen & 0b00000010){
        doDMA(1, 1);//hdma channel 1
    }
    if (hdmaen & 0b00000001){
        doDMA(0, 1);//hdma channel 0
    }
}