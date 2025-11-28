char vals[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

//convert a byte into a string 
void byte_to_str(byte b, char* dest){
    if (!dest) return;
    dest[0] = vals[(unsigned char)b / 16];
    dest[1] = vals[(unsigned char)b % 16];
    dest[2] = '\0';
}

//set and clear individual bits in a 
byte apply_mask(byte val, byte set_bits, byte clear_bits){
    return (val | set_bits) & clear_bits;
}

//select a single bit from a byte
byte get_bit(byte v, byte ind){
    return (v & (1 << ind)) >> ind;
}

//selects a range of bits from the byte
byte get_bits(byte v, byte ind, byte width){
    byte end_zeros = ind - width;
    return v & (((1 << width) - 1) << end_zeros) >> end_zeros;//sorry for the complicated math
}

//convert an integer (0-65535) into a high byte and a low byte
two_bytes separate_bytes(uint16_t n){
    short hi = n & 0xFF00;
    short lo = n & 0x00FF;
    return (two_bytes){(byte)hi, (byte)lo};
}

//converts a two_bytes into an uint16_t
uint16_t two_bytes_to_int(two_bytes b){
    return (uint16_t)(b.h*256+b.l);
}

//add two 16-bit values
two_bytes add_2_2(two_bytes a, two_bytes b){
    unsigned ans = ((a.h * 256 + a.l) + (b.h * 256 + b.l)) % 65536;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}

//add a 16- and an 8- bit value
two_bytes add_2_1(two_bytes a, byte b){
    unsigned ans = ((a.h * 256 + a.l) + b) % 65536;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}

//subtract two 16-bit values
two_bytes sub_2_2(two_bytes a, two_bytes b){
    unsigned ans = ((a.h * 256 + a.l) - (b.h * 256 + b.l)) % 65536;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}

//subtract a 16- and an 8- bit value
two_bytes sub_2_1(two_bytes a, byte b){
    unsigned ans = ((a.h * 256 + a.l) - b) % 65536;
    return (two_bytes){ans & 0xFF00, ans & 0x00FF};
}

//convert two byte values into a single uint16_t
uint16_t bytes_to_int(byte a, byte b){
    return (uint16_t)(a*256+b);
}