namespace nintendo64
{

// TODO: handle floating point exceptions and restrictions
// but we will just ignore them for now

void instr_cvt_d_w(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const s32 word = bit_cast_from_float(read_cop1_reg(n64,fs));

    const f64 d = f64(word);

    write_cop1_reg(n64,fd,d);
}

void instr_cvt_s_w(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const s32 word = bit_cast_from_float(read_cop1_reg(n64,fs));

    const f32 w = f32(word);

    write_cop1_reg(n64,fd,w);
}

void instr_cvt_s_d(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f64 d = read_cop1_reg(n64,fs);
    const f32 w = f32(d);

    write_cop1_reg(n64,fd,w);
}


void instr_trunc_w_s(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f32 s = f32(read_cop1_reg(n64,fs));

    const s32 w = s32(s); 
    const f32 f = bit_cast_float(w);

    write_cop1_reg(n64,fd,f);   
}

// TODO: these need operand checking
void instr_trunc_w_d(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f64 d = f64(read_cop1_reg(n64,fs));

    const s32 w = s32(d); 
    const f32 f = bit_cast_float(w);

    write_cop1_reg(n64,fd,f);   
}

void instr_trunc_l_s(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f32 s = f32(read_cop1_reg(n64,fs));

    const s64 l = s64(s); 
    const f64 f = bit_cast_double(l);

    write_cop1_reg(n64,fd,f);   
}

void instr_trunc_l_d(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f64 d = f64(read_cop1_reg(n64,fs));

    const s64 l = s64(d); 
    const f64 f = bit_cast_double(l);

    write_cop1_reg(n64,fd,f);   
}

void instr_mov_s(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f32 s = f32(read_cop1_reg(n64,fs));

    write_cop1_reg(n64,fd,s);    
}

template<typename FUNC>
void float_s_op(N64& n64, const Opcode& opcode, FUNC func)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);
    const u32 ft = get_ft(opcode);

    const f32 ans = func(f32(read_cop1_reg(n64,fs)),f32(read_cop1_reg(n64,ft)));
    write_cop1_reg(n64,fd,ans);
}


template<typename FUNC>
void float_d_op(N64& n64, const Opcode& opcode, FUNC func)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);
    const u32 ft = get_ft(opcode);

    const f64 ans = func(f64(read_cop1_reg(n64,fs)),f64(read_cop1_reg(n64,ft)));
    write_cop1_reg(n64,fd,ans);
}

void instr_div_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64,opcode,[](f32 v1, f32 v2)
    {
        return v1 / v2;
    });
}

void instr_add_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64,opcode,[](f32 v1, f32 v2)
    {
        return v1 + v2;
    });
}

void instr_sub_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64,opcode,[](f32 v1, f32 v2)
    {
        return v1 - v2;
    });
}

void instr_mul_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64,opcode,[](f32 v1, f32 v2)
    {
        return v1 * v2;
    });    
}

void instr_add_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64,opcode,[](f64 v1, f64 v2)
    {
        return v1 + v2;
    });
}


void instr_sub_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64,opcode,[](f64 v1, f64 v2)
    {
        return v1 - v2;
    });
}

// table 7-11 for cond desc
template<typename FUNC>
void float_cond_s(N64& n64, const Opcode& opcode, FUNC func)
{
    const u32 fs = get_fs(opcode);
    const u32 ft = get_ft(opcode);

    const f32 v1 = f32(read_cop1_reg(n64,fs));
    const f32 v2 = f32(read_cop1_reg(n64,ft));

    // TODO: check inputs are valid

    // write out result of comparison
    n64.cpu.cop1.c = func(v1,v2);
}

void instr_c_le_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64,opcode,[](f32 v1, f32 v2)
    {
        return v1 <= v2;
    });
}

void instr_bc1tl(N64& n64, const Opcode& opcode)
{
    const auto target = compute_branch_addr(n64.cpu.pc,opcode.imm);

    // cond is true
    if(n64.cpu.cop1.c)
    {
        write_pc(n64,target);
    }
    
    // discard delay slot
    else
    {
        skip_instr(n64.cpu);
    }    
}

}