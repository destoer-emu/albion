#include <gb/cpu.h>
#include <gb/memory.h>

namespace gameboy
{

void Cpu::exec_cb(uint8_t cbop)
{
    switch(cbop)
    {
        
        case 0x0: // rlc b
            b = instr_rlc(b);
            break;
            
        case 0x1: // rlc c
            c = instr_rlc(c);
            break;
        
        case 0x2: // rlc d
            d = instr_rlc(d);
            break;
            
        case 0x3: // rlc e
            e = instr_rlc(e);
            break;
            
        case 0x4: // rlc h
            h = instr_rlc(h);
            break;
            
        case 0x5: // rlc l
            l = instr_rlc(l);
            break;
            
        case 0x6: // rlc (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_rlc(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
            
        case 0x7: // rlc a
            a = instr_rlc(a);
            break;
        
        case 0x8: // rrc b
            b = instr_rrc(b);
            break;
            
        case 0x9: // rrc c
            c = instr_rrc(c);
            break;
            
        case 0xa: // rrc d
            d = instr_rrc(d);
            break;
            
        case 0xb: // rrc e
            e = instr_rrc(e);
            break;
        
        case 0xc: // rrc h
            h = instr_rrc(h);
            break;
            
        case 0xd: // rrc l
            l = instr_rrc(l);
            break;
        
        case 0xe: // rrc (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_rrc(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xf: // rrc a
            a = instr_rrc(a);
            break;
            
        case 0x10: // rl b
            b = instr_rl(b);
            break;
        
        case 0x11: // rl c (rotate left)
            c = instr_rl(c);
            break;
        
        case 0x12: // rl d
            d = instr_rl(d);
            break;
            
        case 0x13: // rl e
            e = instr_rl(e);
            break;
            
        case 0x14: // rl h 
            h = instr_rl(h);
            break;
            
        case 0x15: // rl l
            l = instr_rl(l);
            break;
        
        case 0x16: // rl (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_rl(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x17: // rl a
            a = instr_rl(a);
            break;
        
        case 0x18: // rr b
            b = instr_rr(b);
            break;
        
        case 0x19: // rr c
            c = instr_rr(c);
            break;
            
        case 0x1a: // rr d
            d = instr_rr(d);
            break;
        
        case 0x1b: // rr e 
            e = instr_rr(e);
            break;
        
        case 0x1c: // rr h
            h = instr_rr(h);
            break;
        
        case 0x1d: // rr l
            l = instr_rr(l);
            break;
        
        case 0x1e: // rr (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_rr(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x1f: // rr a
            a = instr_rr(a);
            break;
        
        
        case 0x20: // sla b
            b = instr_sla(b);
            break;
            
        case 0x21: // sla c
            c = instr_sla(c);
            break;
        
        case 0x22: // sla d
            d = instr_sla(d);
            break;
        
        case 0x23: // sla e
            e = instr_sla(e);
            break;
            
        case 0x24: // sla h
            h = instr_sla(h);
            break;
            
        case 0x25: // sla l
            l = instr_sla(l);
            break;
        
        case 0x26: // sla (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_sla(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x27: // sla a
            a = instr_sla(a);
            break;
        
        case 0x28: // sra b
            b = instr_sra(b);
            break;
            
        case 0x29: // sra c
            c = instr_sra(c);
            break;
        
        case 0x2a: // sra d
            d = instr_sra(d);
            break;
            
        case 0x2b: // sra e
            e = instr_sra(e);
            break;
            
        case 0x2c: // sra h
            h = instr_sra(h);
            break;
            
        case 0x2d: // sra l
            l = instr_sra(l);
            break;
        
        case 0x2e: // sra (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_sra(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
            
        case 0x2f: // sra a
            a = instr_sra(a);
            break;
        
        case 0x30: // swap b
            b = instr_swap(b);
            break;
            
        case 0x31: // swap c
            c = instr_swap(c);
            break;
            
        case 0x32: // swap d
            d = instr_swap(d);
            break;
        
        case 0x33: // swap e
            e = instr_swap(e);
            break;
        
        case 0x34: // swap h
            h = instr_swap(h);
            break;
            
        case 0x35: // swap l
            l = instr_swap(l);
            break;
            
        case 0x36: // swap (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_swap(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x37: // swap a 
            a = instr_swap(a);
            break;
        
        case 0x38: // srl b
            b = instr_srl(b);
            break;
        
        case 0x39: // srl c
            c = instr_srl(c);
            break;
        
        case 0x3a: // srl d
            d = instr_srl(d);
            break;
            
        case 0x3b: // srl e
            e = instr_srl(e);
            break;
            
        case 0x3c: // srl h
            h = instr_srl(h);
            break;
            
        case 0x3d: // srl l
            l = instr_srl(l);
            break;
        
        case 0x3e: // srl (hl)
            cbop = mem->read_memt(read_hl());
            cbop = instr_srl(cbop);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x3f: // srl a
            a = instr_srl(a);
            break;
        
        case 0x40: // bit 0, b
            instr_bit(b,0);
            break;
        
        case 0x41: // bit 0, c <--- just before the glitched screen
            instr_bit(c,0);
            break;
        
        case 0x42: // bit 0, d
            instr_bit(d,0);
            break;
        
        case 0x43: // bit 0, e
            instr_bit(e,0);
            break;
        
        case 0x44: // bit 0, h
            instr_bit(h,0);
            break;
            
        case 0x45: // bit 0, l
            instr_bit(l,0);
            break;
            
        
        case 0x46: // bit 0, (hl)
            cbop = mem->read_memt(read_hl());
            instr_bit(cbop,0);
            break;
        
        case 0x47: // bit 0, a
            instr_bit(a,0);
            break;
        
        case 0x48: // bit l, b  <--- just before the glitch print
            instr_bit(b,1);
            break;
        
        case 0x49: // bit 1, c
            instr_bit(c,1);
            break;
        
        case 0x4a: // bit 1, d
            instr_bit(d,1);
            break;
        
        case 0x4b: // bit 1, e
            instr_bit(e,1);
            break;
        
        case 0x4c: // bit 1,  h
            instr_bit(h,1);
            break;
            
        case 0x4d: // bit 1, l
            instr_bit(l,1);
            break;
        
        case 0x4e: // bit 1, (hl)
            instr_bit(mem->read_memt(read_hl()),1);
            break;
        
        case 0x4f: // bit 1, a
            instr_bit(a,1);
            break;
        
        case 0x50: // bit 2, b
            instr_bit(b,2);
            break;
            
        case 0x51: // bit  2, c 
            instr_bit(c,2);
            break;	
            
        case 0x52: // bit 2, d 
            instr_bit(d,2);
            break;
        
        case 0x53: // bit 2 , e  
            instr_bit(e,2);
            break;
        
        case 0x54: // bit 2, h  
            instr_bit(h,2);
            break;
        
        case 0x55: // bit 2, l
            instr_bit(l,2);
            break;
        
        case 0x56: // bit 2, (hl)
            instr_bit(mem->read_memt(read_hl()),2);
            break;
        
        case 0x57: // bit 2, a
            instr_bit(a,2);
            break;
        
        
        case 0x58: // bit 3, b
            instr_bit(b,3);
            break;
        
        case 0x59: // bit 3, c  
            instr_bit(c,3);
            break;
        
        case 0x5a: // bit 3, d 
            instr_bit(d,3);
            break;
        
        case 0x5b: // bit 3, e 
            instr_bit(e,3);
            break;
            
        case 0x5c: // bit 3, h 
            instr_bit(h,3);
            break;	
        
        case 0x5d: // bit 3, l
            instr_bit(l,3);
            break;
        
        case 0x5e: // bit 3, (hl)
            instr_bit(mem->read_memt(read_hl()),3);
            break;
        
        
        case 0x5f: // bit 3, a
            instr_bit(a,3);
            break;
        
        case 0x60: // bit 4, b
            instr_bit(b,4);
            break;



        case 0x61: // bit 4, c
            instr_bit(c,4);
            break;
        
        case 0x62: // bit  4, d 
            instr_bit(d,4);
            break;
        
        case 0x63: // bit 4, e 
            instr_bit(e ,4);
            break;
        
        case 0x64: // bit 4, h 
            instr_bit(h,4);
            break;
        
        case 0x65: // bit 4, l
            instr_bit(l,4);
            break;
        
        case 0x66: // bit 4, (hl)
            instr_bit(mem->read_memt(read_hl()),4);
            break;
        
        case 0x67: // bit 4, a  
            instr_bit(a,4);
            break;
        
        case 0x68: // bit 5, b
            instr_bit(b,5);
            break;
        
        case 0x69: // bit 5, c 
            instr_bit(c,5);
            break;
        
        case 0x6a: //bit 5, d 
            instr_bit(d,5);
            break;
            
        case 0x6b: // bit 5, e  
            instr_bit(e,5);
            break;
        
        case 0x6c: // bit 5, h 
            instr_bit(h,5);
            break;
        
        
        case 0x6d: // bit 5, l
            instr_bit(l,5);
            break;
        
        case 0x6e: // bit 5, (hl) 
            instr_bit(mem->read_memt(read_hl()),5);
            break;
        
        case 0x6f: // bit 5, a
            instr_bit(a,5);
            break;
        
        case 0x70: // bit 6, b 
            instr_bit(b,6);
            break;
        
        case 0x71: // bit 6, c 
            instr_bit(c,6);
            break;
        
        case 0x72: // bit 6, d  
            instr_bit(d,6);
            break;
        
        case 0x73: // bit  6, e
            instr_bit(e,6);
            break;

        case 0x74: // bit 6, h  
            instr_bit(h,6);
            break;	
            
        case 0x75: // bit 6, l 
            instr_bit(l,6);
            break;
        
        case 0x76: // bit 6, (hl)
            instr_bit(mem->read_memt(read_hl()),6);
            break;
        
        case 0x77: // bit 6, a
            instr_bit(a,6);
            break;
            
        case 0x78: // bit  7, b
            instr_bit(b,7);
            break;
        
        case 0x79: // bit 7, c  
            instr_bit(c,7);
            break;
        
        case 0x7a: // bit 7, d  
            instr_bit(d,7);
            break;
        
        case 0x7b: // bit bit 7, e  
            instr_bit(e,7);
            break;
        
        case 0x7c: // bit 7, h
            instr_bit( h, 7);
            break;
        
        case 0x7d: // bit 7, l
            instr_bit(l,7);
            break;


        case 0x7e: // bit 7, (hl)
            cbop = mem->read_memt(read_hl());
            instr_bit(cbop,7);
            break;
            
        case 0x7f: // bit 7, a
            instr_bit(a, 7);
            break;


        case 0x80: // res 0, b
            b = deset_bit(b,0);
            break;

        case 0x81: // res  0, c 
            c = deset_bit(c,0);
            break;
            
        case 0x82: // res 0, d  
            d = deset_bit(d,0);
            break;	
        
        case 0x83: // res 0, e  
            e = deset_bit(e,0);
            break;
        
        case 0x84: // res 0, h 
            h = deset_bit(h,0);
            break;

        case 0x85: // res 0, l  
            l = deset_bit(l,0);
            break;	
            
        case 0x86: // res 0, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,0);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x87: //res 0,a
            a = deset_bit(a,0);
            break;
        
        case 0x88: // res 1 ,b
            b = deset_bit(b,1);
            break;
        
        case 0x89: // res 1, c
            c = deset_bit(c,1);
            break;
            
        case 0x8a: // res 1, d  
            d = deset_bit(d,1);
            break;
            
        case 0x8b: // res 1, e
            e = deset_bit(e,1);
            break;
        
        
        case 0x8c: // res  1, h  
            h = deset_bit(h,1);
            break;
        
        case 0x8d: // res 1, l
            l = deset_bit(l,1);
            break;
        
        
        case 0x8e: // res 1, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,1);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x8f: // res 1, a 
            a = deset_bit(a,1);
            break;
        
        case 0x90: // res 2, b
            b = deset_bit(b,2);
            break;
        
        case 0x91: // res 2, c  
            c = deset_bit(c,2);
            break;
        
        case 0x92: // res 2, d  
            d = deset_bit(d,2);
            break;
        
        case 0x93: // res 2, e 
            e = deset_bit(e,2);
            break;
        
        case 0x94: // res 2, h  
            h = deset_bit(h,2);
            break;
        
        case 0x95: // res 2, l  
            l = deset_bit(l,2);
            break;
        
        case 0x96: // res 2, (hl) 
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,2);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0x97: // res 2, a  
            a = deset_bit(a,2);
            break;
            
        case 0x98: // res 3, b
            b = deset_bit(b,3);
            break;
        
        case 0x99: // res 3, c 
            c = deset_bit(c,3);
            break;
        
        
        case 0x9a: // res 3, d 
            d = deset_bit(d,3);
            break;
        
        case 0x9b: // res 3, e  
            e = deset_bit(e,3);
            break;
        
        case 0x9c: // res 3, h 
            h = deset_bit(h,3);
            break;
        
        case 0x9d: // res 3, l
            l = deset_bit(l,3);
            break;
        

        
        case 0x9e: // res 3, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,3);
            mem->write_memt(read_hl(),cbop);
            break; 				
        
        case 0x9f: // res 3, a
            a = deset_bit(a,3);
            break;
        
        case 0xa0: // res 4, b 
            b = deset_bit(b,4);
            break;
        
        case 0xa1: // res 4, c  
            c = deset_bit(c,4);
            break;
        
        case 0xa2: // res  4, d 
            d = deset_bit(d,4);
            break;
        
        case 0xa3: // res 4, e 
            e = deset_bit(e,4);
            break;
        
        case 0xa4: // res 4, h  
            h = deset_bit(h,4);
            break;
        
        case 0xa5: // res 4, l  
            l = deset_bit(l,4);
            break;
            
        case 0xa6: // res 4, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,4);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xa7: // res 4, a 
            a = deset_bit(a,4);
            break;
        
        case 0xa8: // res 5, b  
            b = deset_bit(b,5);
            break;
        
        case 0xa9: // res 5, c  
            c = deset_bit(c,5);
            break;
        
        case 0xaa: // res 5, d  
            d = deset_bit(d,5);
            break;
        
        case 0xab: // res 5, e
            e = deset_bit(e,5);
            break;
        
        case 0xac: // res 5, h 
            h = deset_bit(h,5);
            break;
        
        case 0xad: // res 5, l
            l = deset_bit(l,5);
            break;
        
        case 0xae: // res 5, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,5);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xaf: // res 5, a 
            a = deset_bit(a,5);
            break;
        
        case 0xb0: // res 6, b  
            b = deset_bit(b,6);
            break;
        
        case 0xb1: // res 6, c  
            c = deset_bit(c,6);
            break;
        
        case 0xb2: // res 6, d 
            d = deset_bit(d,6);
            break;
        
        case 0xb3: // res 6, e  
            e = deset_bit(e,6);
            break;
        
        case 0xb4: // res 6, h
            h = deset_bit(h,6);
            break;
        
        case 0xb5: // res 6, l 
            l = deset_bit(l,6);
            break;
        
        case 0xb6: // res 6, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,6);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xb7: // res 6,a 
            a = deset_bit(a,6);
            break;
        
        case 0xb8: // res 7, b  
            b = deset_bit(b,7);
            break;
        
        case 0xb9: // res 7, c 
            c = deset_bit(c,7);
            break;
            
        case 0xba: // res 7, d 
            d = deset_bit(d,7);
            break;
            
        case 0xbb: // res 7, e
            e = deset_bit(e,7);
            break;
        
        case 0xbc: // res 7, h  
            h = deset_bit(h,7);
            break;
        
        case 0xbd: // res 7, l  
            l = deset_bit(l,7);
            break;
        
        case 0xbe: // res 7, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = deset_bit(cbop,7);
            mem->write_memt(read_hl(),cbop);
            break; 
        
        case 0xbf: // res 7, a  
            a = deset_bit(a,7);
            break;
        
        case 0xc0: // set 0, b
            b = set_bit(b,0);
            break;
        
        case 0xc1: // set 0,c 
            c = set_bit(c,0);
            break;
        
        case 0xc2: // set 0,d 
            d = set_bit(d,0);
            break;
            
        case 0xc3: // set 0,e
            e = set_bit(e,0);
            break;
        
        case 0xc4: // set 0, h 
            h = set_bit(h,0);
            break;
        
        case 0xc5: // set 0, l
            l = set_bit(l,0);
            break;
        
        case 0xc6: // set 0, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,0);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xc7: // set 0, a 
            a = set_bit(a,0);
            break;
        
        case 0xc8: // set 1, b 
            b = set_bit(b,1);
            break;
        
        case 0xc9: // set 1, c 
            c = set_bit(c,1);
            break;
        
        case 0xca: // set 1, d 
            d = set_bit(d,1);
            break;
        
        case 0xcb: // set  1, e 
            e = set_bit(e ,1);
            break;
        
        case 0xcc: // set 1, h 
            h = set_bit(h,1);
            break;
        
        case 0xcd: // set l, l
            l = set_bit(l,1);
            break;
        
        case 0xce: // set 1, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,1);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xcf: // set 1, a 
            a = set_bit(a,1);
            break;
        
        case 0xd0: // set 2 ,b
            b = set_bit(b,2);
            break;
        
        case 0xd1: // set 2, c 
            c = set_bit(c,2);
            break;
        
        case 0xd2: // set 2, d 
            d = set_bit(d,2);
            break;
        
        case 0xd3: // set 2 , e 
            e = set_bit(e,2);
            break;
        
        case 0xd4: // set 2, h 
            h = set_bit(h,2);
            break;
        
        case 0xd5: // set 2, l
            l = set_bit(l,2);
            break;
            
        case 0xd6: // set 2, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,2);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xd7: // set 2 a 
            a = set_bit(a,2);
            break;
            
        case 0xd8: // set 3, b
            b = set_bit(b,3);
            break;
        
        case 0xd9: // set 3, c 
            c = set_bit(c,3);
            break;
        
        case 0xda: // set 3, d 
            d = set_bit(d,3);
            break;
        
        case 0xdb: // set 3, e
            e = set_bit(e,3);
            break;
        
        case 0xdc: // set 3, h
            h = set_bit(h,3);
            break;
        
        case 0xdd: // set 3, l
            l = set_bit(l,3);
            break;
        
        case 0xde: // set 3, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,3);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xdf: // set 3, a 
            a = set_bit(a,3);
            break;
        
        case 0xe0: // set 4, b 
            b = set_bit(b,4);
            break;
        
        case 0xe1: // set 4, c
            c = set_bit(c,4);
            break;
            
        case 0xe2: // set 4, d 
            d = set_bit(d,4);
            break;
        
        case 0xe3: // set 4, e 
            e = set_bit(e,4);
            break;
        
        case 0xe4: // set 4 , h 
            h = set_bit(h,4);
            break;
        
        case 0xe5: // set 4 , l 
            l = set_bit(l,4);
            break;
        
        case 0xe6: // set 4, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,4);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xe7: // set 4, a 
            a = set_bit(a,4);
            break;
        
        case 0xe8: // set 5, b 
            b = set_bit(b,5);
            break;
        
        case 0xe9: // set 5, c 
            c = set_bit(c,5);
            break;
        
        case 0xea: //set 5, d
            d = set_bit(d,5);
            break;
        
        case 0xeb: // set 5, e 
            e = set_bit(e,5);
            break;
        
        case 0xec: // set 5, h 
            h = set_bit(h,5);
            break;
        
        case 0xed: // set 5 , l
            l = set_bit(l,5);
            break;
        
        case 0xee: // set 5, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,5);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xef: // set 5, a
            a = set_bit(a,5);
            break;
        
        case 0xf0: // set 6, b 
            b = set_bit(b,6);
            break;
        
        case 0xf1: // set 6, c 
            c = set_bit(c,6);
            break;
        
        case 0xf2: // set 6, d 
            d = set_bit(d,6);
            break;
        
        case 0xf3: // set 6, e 
            e = set_bit(e,6);
            break;
        
        case 0xf4: // set 6, h 
            h = set_bit(h,6);
            break;
        
        case 0xf5: // set 6, l
            l = set_bit(l,6);
            break;
        
        case 0xf6: // set 6, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,6);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xf7: // set 6, a 
            a = set_bit(a,6);
            break;
        
        case 0xf8: // set 7, b  
            b = set_bit(b,7);
            break;
            
        case 0xf9: // set 7, c 	
            c = set_bit(c,7);
            break;
            
        case 0xfa: // set 7, d 
            d = set_bit(d,7);
            break;	
        
        case 0xfb: // set 7 , e 
            e = set_bit(e,7);
            break;
        
        case 0xfc: // set 7, h
            h = set_bit(h,7);
            break;
        
        case 0xfd: // set 7, l
            l = set_bit(l,7);
            break;
        
        case 0xfe: // set 7, (hl)
            cbop = mem->read_memt(read_hl());
            cbop = set_bit(cbop,7);
            mem->write_memt(read_hl(),cbop);
            break;
        
        case 0xff: // set 7,a 
            a = set_bit(a,7);
            break;

        default:
        {
			printf("invalid opcode %x\n",cbop);
			throw std::runtime_error("invalid opcode!");	            
        }
    }    
}

}