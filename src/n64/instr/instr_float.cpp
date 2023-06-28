namespace nintendo64
{

// TODO: handle floating point exceptions and restrictions
// but we will just ignore them for now

void instr_cvt_d_w(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const u32 word = bit_cast_from_float(read_cop1_reg(n64,fs));

    const f64 d = f64(word);

    write_cop1_reg(n64,fd,d);
}

void instr_cvt_s_w(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const u32 word = bit_cast_from_float(read_cop1_reg(n64,fs));

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


template<typename FUNC>
void float_s_op(N64& n64, const Opcode& opcode, FUNC func)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);
    const u32 ft = get_ft(opcode);

    const f32 ans = func(f32(read_cop1_reg(n64,fs)),f32(read_cop1_reg(n64,ft)));
    write_cop1_reg(n64,fd,ans);
}

f32 float_div_s(f32 v1, f32 v2)
{
    return v1 / v2;
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