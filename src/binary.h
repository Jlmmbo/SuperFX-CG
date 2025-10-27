typedef unsigned char byte;

//typedef unsigned long int uint16_t;

typedef struct{
    byte h:8;
    byte l:8;
}two_bytes;

/*typedef struct {
    byte h;
    byte m;
    byte l;
}three_bytes;*/

/*convert an integer (0-65535) into a high byte and a low byte*/
two_bytes separate_bytes(short n){
    short hi = n & 0xFF00;
    short lo = n & 0x00FF;
    return (two_bytes){(byte)hi, (byte)lo};
}

int two_bytes_to_int(two_bytes b){
    return (int)(b.h*255+b.l);
}

two_bytes add_2_2(two_bytes a, two_bytes b){
    unsigned a_int = a.h * 256 + a.l;
    unsigned b_int = b.h * 256 + b.l;
    unsigned ans = (a_int + b_int) % 65536;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}

two_bytes add_2_1(two_bytes a, byte b){
    unsigned a_int = a.h * 256 + a.l;
    unsigned ans = (a_int + b) % 65535;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}

two_bytes sub_2_2(two_bytes a, two_bytes b){
    unsigned a_int = a.h * 256 + a.l;
    unsigned b_int = b.h * 256 + b.l;
    unsigned ans = (a_int - b_int) % 65536;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}

two_bytes sub_2_1(two_bytes a, byte b){
    unsigned a_int = a.h * 256 + a.l;
    unsigned ans = (a_int - b) % 65535;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}


int bytes_to_int(byte a, byte b){
    return (int)(a*255+b);
}