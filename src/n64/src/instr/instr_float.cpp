namespace nintendo64
{

// TODO: handle floating point exceptions and restrictions
// but we will just ignore them for now

template<typename FUNC>
void instr_cvt(N64& n64, const Opcode& opcode, FUNC func)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f64 out = func(read_cop1_reg(n64,fs));

    write_cop1_reg(n64,fd,out);    
}

void instr_cvt_d_w(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const s32 word = bit_cast_from_float(in);
        return f64(word);
    });
}

void instr_cvt_d_l(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const s64 l = bit_cast_from_double(in);
        return f64(l);
    });
}

void instr_cvt_d_s(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const f32 s = f32(in);
        return f64(s);        
    });
}


void instr_cvt_l_d(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const s64 l = s64(in);
        return bit_cast_double(l);        
    });
}


void instr_cvt_l_s(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const f32 s = f32(in);
        const s64 l = s64(s);

        return bit_cast_double(l);
    });
}

void instr_cvt_s_w(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const s32 word = bit_cast_from_float(in);
        const f32 s = f32(word);

        return s;
    });
}

void instr_cvt_s_d(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        return f32(in);
    });
}

void instr_cvt_s_l(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const s64 l = bit_cast_from_double(in);
        return f32(l);
    });
}

void instr_cvt_w_d(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const s32 w = s32(in);

        return f64(bit_cast_float(w));
    });
}

void instr_cvt_w_s(N64& n64, const Opcode& opcode)
{
    instr_cvt(n64,opcode,[](f64 in)
    {
        const f32 s = f32(in);
        const s32 w = s32(s);

        return f64(bit_cast_float(w));
    });
}

template<typename FUNC>
void instr_trunc(N64& n64, const Opcode& opcode, FUNC func)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    const f64 out = func(read_cop1_reg(n64,fs));

    write_cop1_reg(n64,fd,out);    
}

void instr_trunc_w_s(N64& n64, const Opcode& opcode)
{
    instr_trunc(n64,opcode,[](f64 in)
    {
        const f32 s = f32(in);
        const s32 w = s32(s);

        return bit_cast_float(w);
    });
}

void instr_trunc_w_d(N64& n64, const Opcode& opcode)
{
    instr_trunc(n64,opcode,[](f64 in)
    {
        const s32 w = s32(in);

        return bit_cast_float(w);
    });
}


void instr_trunc_l_s(N64& n64, const Opcode& opcode)
{
    instr_trunc(n64,opcode,[](f64 in)
    {
        const f32 s = f32(in);
        const s64 l = s64(s);

        return bit_cast_double(l);
    });
}


void instr_trunc_l_d(N64& n64, const Opcode& opcode)
{
    instr_trunc(n64,opcode,[](f64 in)
    {
        const s64 l = s64(in);

        return bit_cast_double(l);
    });
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

    const f64 ans = func(f32(read_cop1_reg(n64,fs)),f32(read_cop1_reg(n64,ft)));
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

void instr_sqrt_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return std::sqrt(f);
    });
}

void instr_abs_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return std::abs(f);
    });
}

void instr_neg_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return -f;
    });
}

void instr_round_l_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return bit_cast_double(long(std::nearbyint((f))));
    });
}

void instr_ceil_l_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return bit_cast_double(long(std::ceil(f)));
    });
}

void instr_floor_l_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return bit_cast_double(long(std::floor(f)));
    });
}

void instr_round_w_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return bit_cast_float(int(std::nearbyint(f )));
    });
}

void instr_ceil_w_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return bit_cast_float(int(std::ceil(f)));
    });
}

void instr_floor_w_s(N64& n64, const Opcode& opcode)
{
    float_s_op(n64, opcode, [](f32 f, f32 unused)
    {
        UNUSED(unused);
        return bit_cast_float(int(std::floor(f)));
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

void instr_mul_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64,opcode,[](f64 v1, f64 v2)
    {
        return v1 * v2;
    });
}

void instr_div_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64,opcode,[](f64 v1, f64 v2)
    {
        return v1 / v2;
    });
}

void instr_mov_d(N64& n64, const Opcode& opcode)
{
    const u32 fs = get_fs(opcode);
    const u32 fd = get_fd(opcode);

    write_cop1_reg(n64,fd,read_cop1_reg(n64,fs));    
}

void instr_sqrt_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return std::sqrt(f);
    });
}

void instr_abs_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return std::abs(f);
    });
}

void instr_neg_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return -f;
    });
}

void instr_roundl_d(N64& n64, const Opcode& opcode) {
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return bit_cast_double(long(std::nearbyint(f))); // round to nearest, tiebreak towards the even number
    });
}

void instr_ceil_l_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return bit_cast_double(long(std::ceil(f)));
    });
}

void instr_floor_l_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return bit_cast_double(long(std::floor(f)));
    });
}

void instr_round_w_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return bit_cast_float(int(std::nearbyint(f))); // truncate to float, round, cast up to double
    });
}

void instr_ceil_w_d(N64& n64, const Opcode& opcode) {
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return bit_cast_float(int(std::ceil(f)));
    });
}

void instr_floor_w_d(N64& n64, const Opcode& opcode)
{
    float_d_op(n64, opcode, [](f64 f, f64 unused)
    {
        UNUSED(unused);
        return bit_cast_float(int(std::floor(f)));
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

template<typename FUNC>
void float_cond_d(N64& n64, const Opcode& opcode, FUNC func)
{
    const u32 fs = get_fs(opcode);
    const u32 ft = get_ft(opcode);

    const f64 v1 = f64(read_cop1_reg(n64,fs));
    const f64 v2 = f64(read_cop1_reg(n64,ft));

    // TODO: check inputs are valid

    // write out result of comparison
    n64.cpu.cop1.c = func(v1,v2);
}

void instr_c_ole_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64,opcode,[](f32 v1, f32 v2)
    {
        return v1 <= v2;
    });
}

void instr_c_ole_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64,opcode,[](f64 v1, f64 v2)
    {
        return v1 <= v2;
    });
}

void instr_c_olt_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64,opcode,[](f64 v1, f64 v2)
    {
        return v1 < v2;
    });
}

void instr_c_eq_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2) {
        return v1 == v2;
    });
}

void instr_c_eq_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return v1 == v2;
    });
}

void instr_c_f_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        UNUSED(v1); UNUSED(v2);
        return false;
    });
}

void instr_c_f_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        UNUSED(v1); UNUSED(v2);
        return false;
    });
}

bool isnan_f(f32 f) {
    return std::isnan(f);
}

bool isnan_d(f64 f) {
    return f == std::numeric_limits<double>::quiet_NaN();
}

void instr_c_un_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return isnan_d(v1) || isnan_d(v2);
    });
}

void instr_c_un_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return isnan_f(v1) || isnan_f(v2);
    });
}

void instr_c_ueq_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || (v1 == v2);
    });
}

void instr_c_ueq_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return (isnan_f(v1) || isnan_f(v2)) || (v1 == v2);
    });
}

void instr_c_olt_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return v1 < v2;
    });
}

void instr_c_ult_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || v1 < v2;
    });
}

void instr_c_ult_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return (isnan_f(v1) || isnan_f(v2)) || v1 < v2;
    });
}

void instr_c_ule_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return (isnan_f(v1) || isnan_f(v2)) || (v1 < v2) || (v1 == v2);
    });
}

void instr_c_ule_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || (v1 < v2) || (v1 == v2);
    });
}


// TODO: these all exception on nan
void instr_c_sf_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        UNUSED(v1); UNUSED(v2);
        return false;
    });
}

void instr_c_ngle_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return isnan_d(v1) || isnan_d(v2);
    });
}

void instr_c_seq_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return v1 == v2;
    });
}

void instr_c_ngl_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || v1 == v2;
    });
}

void instr_c_lt_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return v1 < v2;
    });
}

void instr_c_nge_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || v1 < v2;
    });
}

void instr_c_le_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return v1 < v2 || v1 == v2;
    });
}

void instr_c_ngt_d(N64& n64, const Opcode& opcode)
{
    float_cond_d(n64, opcode, [](f64 v1, f64 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || v1 < v2 || v1 == v2;
    });
}

void instr_c_sf_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
    UNUSED(v1); UNUSED(v2);
       return false;
    });
}

void instr_c_ngle_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
       return (isnan_d(v1) || isnan_d(v2));
    });
}

void instr_c_seq_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return v1 == v2;
    });
}

void instr_c_ngl_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || v1 == v2;
    });
}

void instr_c_lt_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return v1 < v2;
    });
}

void instr_c_nge_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return (isnan_d(v1) || isnan_d(v2)) || v1 < v2;
    });
}

void instr_c_le_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
        return v1 < v2 || v1 == v2;
    });
}

void instr_c_ngt_s(N64& n64, const Opcode& opcode)
{
    float_cond_s(n64, opcode, [](f32 v1, f32 v2)
    {
       return (isnan_d(v1) || isnan_d(v2)) || v1 < v2 || v1 == v2;
    });
}

void instr_bc1tl(N64& n64, const Opcode& opcode)
{
    instr_branch_likely(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        UNUSED(opcode);
        return n64.cpu.cop1.c;
    });   
}

void instr_bc1fl(N64& n64, const Opcode& opcode)
{
    instr_branch_likely(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        UNUSED(opcode);
        return !n64.cpu.cop1.c;
    });    
}

void instr_bc1t(N64& n64, const Opcode& opcode)
{
    instr_branch(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        UNUSED(opcode);
        return n64.cpu.cop1.c;
    }); 
}

void instr_bc1f(N64& n64, const Opcode& opcode)
{
    instr_branch(n64,opcode,[](N64& n64, const Opcode& opcode)
    {
        UNUSED(opcode);
        return !n64.cpu.cop1.c;
    }); 
}

}