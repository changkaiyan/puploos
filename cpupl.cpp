/**
 * 指令功能模拟级计算机:只有在内存和立即数时会出现有符号数，其余全部是无符号数。
 * 实现RV32IM指令集：乘除法、整数操作
 * 关闭全局中断后，中断标志位仍然有效
 * 实现用户级ISA和特权级ISA中的机器模式:特权模式v. 20190608版本
 */
#include <iostream>
#include <pthread.h>
#include <assert.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <limits>
#include <termio.h>
#include <exception>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <unistd.h>
#include "device.hpp"
#include "cpupl.h"
struct termios stored_settings;
struct termios origin_settings;
struct termios new_settings;
int server_sockfd = -1;
int client_sockfd = -1;
unsigned time_counter[2]{0u, DEFAULT_TIMECMP}; //定时器寄存器初始化
unsigned csr[CSR_NUMBER]{};                    

/**
 * @brief 打印关键寄存器的调试器
 * 
 * @param pc 程序指针
 * @param ir 指令寄存器
 * @param x 32个整数寄存器
 * @param csr csr控制寄存器
 * @param time_counter 
 * @param fr 
 */
void debug_print(unsigned pc, unsigned ir, int x[], unsigned csr[], unsigned time_counter[], Floating fr[])
{

    FLUSH("--------------------DEBUG PRINT @0x%08x-----------------\n", ir);
    FLUSH("PC = [0x%x], IR = [0x%08x], OP = [0x%x]\n", pc, ir, OPCODE(ir));
    for (int i{1}; i < 32; ++i)
    {
        if (i % 10 != 0)
            FLUSH("x[%d] = 0x%x, ", i, x[i]);
        else
            FLUSH("x[%d] = 0x%x\n", i, x[i]);
    }
    for (int i{1}; i < 32; ++i)
    {
        if (i % 10 != 0)
            FLUSH("\nfr[%d] = float: %f, double: %lf ", i, fr[i].flo, fr[i].dou);
        else
            FLUSH("\nfr[%d] = float: %f, double: %lf", i, fr[i].flo, fr[i].dou);
    }
    FLUSH("\nmie = [0x%x], mip = [0x%x], mcause = [0x%x], mepc = [0x%x], mstatus = [0x%x]\n",
          csr[A_MIE], csr[A_MIP], csr[A_MCAUSE], csr[A_MEPC], csr[A_MSTATUS]);
    FLUSH("mtvec = [0x%x], mtval = [0x%x], mtime = [%u], mtimecmp = [%u], misa = [0x%x], minstret = [%d], satp = [0x%x]\n",
          csr[A_MTVEC], csr[A_MTVAL], time_counter[A_MTIME], time_counter[A_MTIMECMP], csr[A_MISA], csr[A_MINSTRET], csr[A_SATP]);
    FLUSH("SIE = [%d], SIP = [%d], TIE = [%d], TIP = [%d], EIE = [%d], EIP = [%d]\n",
          (csr[A_MIE] & MSIE(1)) != 0, (csr[A_MIP] & MSIP(1)) != 0, (csr[A_MIE] & MTIE(1)) != 0, (csr[A_MIP] & MTIP(1)) != 0,
          (csr[A_MIE] & MEIE(1)) != 0, (csr[A_MIP] & MEIP(1)) != 0);

    FLUSH("-----------------------DEBUG PRINT END------------------------\n");
}

void cpu_start()
{
    //! 处理机内部状态初始化区
    unsigned program_count{};   //程序指针
    unsigned instruction_reg{}; //指令寄存器
    bool illegal_inst{};        //指令不合法
    int x[REG_NUMBER]{};        //riscv中禁止使用x[0]
    Floating fr[32]{};          //浮点寄存器
    unsigned fcs{};             //浮点控制和状态寄存器

    csr[A_MARCHID] = 64;                      //内存容量
    csr[A_MTVEC] = INTERPROGRAM;              //中断寄存器初始化
    csr[A_MIMPID] = VERSION;                  // 第一版CPU设计
    csr[A_MISA] = DEFAULT_ISA;                // 默认指令集
    csr[A_MSTATUS] = MIE(1) | MPIE(1);        //初始化全局中断使能
    csr[A_MIE] = MEIE(1) | MTIE(1) | MSIE(1); //初始化外部中断、定时器中断、软件中断

    //! 处理机启动循环
    while (true)
    {
        try
        {
            instruction_reg = mem[program_count]; //取指令
            if (time_counter[A_MTIME] < time_counter[A_MTIMECMP])
                time_counter[A_MTIME]++;
            else
            {
                //定时器中断
                csr[A_MIP] |= MTIP(1); //定时器中断挂起
            }

            program_count += 4;

            if (time_counter[A_MTIME] < time_counter[A_MTIMECMP])
                time_counter[A_MTIME]++;
            else
            {
                //定时器中断
                csr[A_MIP] |= MTIP(1); //定时器中断挂起
            }
            if (IS_AND(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] & x[RS2(instruction_reg)];
            }
            else if (IS_ANDI(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] & S_I_IMM(instruction_reg);
            }
            else if (IS_ADD(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] + x[RS2(instruction_reg)];
            }
            else if (IS_ADDI(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] + S_I_IMM(instruction_reg);
            }
            else if (IS_AUIPC(instruction_reg))
            {
                // riscv中的pc是当前地址，我们cpu的pc是指下一条指令的地址。
                x[RD(instruction_reg)] = program_count + S_U_IMM(instruction_reg) - 4;
            }
            else if (IS_BEQ(instruction_reg))
            {
                program_count += x[RS1(instruction_reg)] == x[RS2(instruction_reg)] ? S_B_IMM(instruction_reg) - 4 : 0;
            }
            else if (IS_BGE(instruction_reg))
            {
                program_count += x[RS1(instruction_reg)] >= x[RS2(instruction_reg)] ? S_B_IMM(instruction_reg) - 4 : 0;
            }
            else if (IS_BGEU(instruction_reg))
            {
                program_count += unsigned(x[RS1(instruction_reg)]) >= unsigned(x[RS2(instruction_reg)]) ? S_B_IMM(instruction_reg) - 4 : 0;
            }
            else if (IS_BLT(instruction_reg))
            {
                program_count += x[RS1(instruction_reg)] < x[RS2(instruction_reg)] ? S_B_IMM(instruction_reg) - 4 : 0;
            }
            else if (IS_BLTU(instruction_reg))
            {
                program_count += unsigned(x[RS1(instruction_reg)]) < unsigned(x[RS2(instruction_reg)]) ? S_B_IMM(instruction_reg) - 4 : 0;
            }
            else if (IS_BNE(instruction_reg))
            {
                program_count += unsigned(x[RS1(instruction_reg)]) != unsigned(x[RS2(instruction_reg)]) ? S_B_IMM(instruction_reg) - 4 : 0;
            }
            else if (IS_CSRRC(instruction_reg))
            {
                auto tmp = csr[I_IMM(instruction_reg)];
                csr[I_IMM(instruction_reg)] = tmp & ~x[RS1(instruction_reg)];
                x[RD(instruction_reg)] = tmp;
            }
            else if (IS_CSRRCI(instruction_reg))
            {
                auto tmp = csr[I_IMM(instruction_reg)];
                csr[I_IMM(instruction_reg)] = tmp & ~RS1(instruction_reg);
                x[RD(instruction_reg)] = tmp;
            }
            else if (IS_CSRRS(instruction_reg))
            {

                auto tmp = csr[I_IMM(instruction_reg)];
                csr[I_IMM(instruction_reg)] = tmp | x[RS1(instruction_reg)];
                x[RD(instruction_reg)] = tmp;
            }
            else if (IS_CSRRSI(instruction_reg))
            {

                auto tmp = csr[I_IMM(instruction_reg)];
                csr[I_IMM(instruction_reg)] = tmp | RS1(instruction_reg);
                x[RD(instruction_reg)] = tmp;
            }
            else if (IS_CSRRW(instruction_reg))
            {
                auto tmp = csr[I_IMM(instruction_reg)];
                csr[I_IMM(instruction_reg)] = x[RS1(instruction_reg)];
                x[RD(instruction_reg)] = tmp;
            }
            else if (IS_CSRRWI(instruction_reg))
            {
                x[RD(instruction_reg)] = csr[I_IMM(instruction_reg)];
                csr[I_IMM(instruction_reg)] = RS1(instruction_reg);
            }
            else if (IS_EBREAK(instruction_reg))
            {
                auto save = csr[A_MIP]; //防止此时键盘输入被当作中断接收
                debug_print(program_count, instruction_reg, x, csr, time_counter, fr);
                int i{};
                board.boardbuf = 0;
                while (board.boardbuf != 'n' and board.boardbuf != 'q')
                {
                    if (i == 0)
                    {
                        FLUSH("Tape n and go to the next instruction or q for quit.\n");
                        i = 1;
                    }
                    sleep(0.5);
                }
                if (board.boardbuf == 'q')
                {
                    board.boardbuf = 0;
                    break;
                }
                board.boardbuf = 0;
                csr[A_MIP] = save;
            }
            else if (IS_ECALL(instruction_reg))
            {
                
                csr[A_MIP] |= MSIP(1);
            }
            else if (IS_FENCE(instruction_reg))
            {
                // 同步内存和IO（非流水线，此处不用同步）
            }
            else if (IS_FENCEI(instruction_reg))
            {
                // 同步指令流 （非乱序执行，此处不用同步）
            }
            else if (IS_JAL(instruction_reg))
            {

                x[RD(instruction_reg)] = program_count;
                program_count += S_J_IMM(instruction_reg) - 4;
            }
            else if (IS_JALR(instruction_reg))
            {

                auto tmp = program_count;
                program_count = (x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)) & (~1);
                x[RD(instruction_reg)] = tmp;
            }
            else if (IS_LB(instruction_reg))
            {
                unsigned rol = (((x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)) & 0b11) << 3);
                x[RD(instruction_reg)] = (int(((mem[S_I_IMM(instruction_reg) + x[RS1(instruction_reg)]] >> rol) & 0xff) << 24) >> 24);
            }
            else if (IS_LBU(instruction_reg))
            {
                //取不到一个字的指令要移位寻找
                unsigned rol = (((x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)) & 0b11) << 3);
                x[RD(instruction_reg)] = (mem[x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)] >> rol) & 0xff;
            }
            else if (IS_LH(instruction_reg))
            {
                unsigned rol = (((x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)) & 0b11) << 3);
                x[RD(instruction_reg)] = (int(((mem[x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)] >> rol) & 0xffff) << 16) >> 16);
            }
            else if (IS_LHU(instruction_reg))
            {
                unsigned rol = (((x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)) & 0b11) << 3);
                x[RD(instruction_reg)] = (mem[x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)] >> rol) & 0xffff;
            }
            else if (IS_LW(instruction_reg))
            {
                x[RD(instruction_reg)] = mem[x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)];
            }
            else if (IS_LUI(instruction_reg))
            {
                x[RD(instruction_reg)] = S_U_IMM(instruction_reg);
            }
            else if (IS_OR(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] | x[RS2(instruction_reg)];
            }
            else if (IS_ORI(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] | S_I_IMM(instruction_reg);
            }
            else if (IS_SB(instruction_reg))
            {
                unsigned tmp = mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)];
                unsigned rol = (((x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)) & 0b11) << 3);
                unsigned one = (0xff << rol);
                tmp &= ~one;
                tmp |= ((x[RS2(instruction_reg)] & 0xff) << rol);
                mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)] = tmp;
                // mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)] = (x[RS2(instruction_reg)] & 0xff);
            }
            else if (IS_SH(instruction_reg))
            {
                unsigned tmp = mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)];
                unsigned rol = (((x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)) & 0b11) << 3);
                unsigned one = (0xffff << rol);
                tmp &= ~one;
                tmp |= ((x[RS2(instruction_reg)] & 0xffff) << rol);
                // mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)] = x[RS2(instruction_reg)] & 0xffff;
            }
            else if (IS_SW(instruction_reg))
            {
                mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)] = x[RS2(instruction_reg)] & 0xffffffff;
            }
            else if (IS_SLL(instruction_reg))
            {
                x[RD(instruction_reg)] = (x[RS1(instruction_reg)] << x[RS2(instruction_reg)]);
            }
            else if (IS_SLLI(instruction_reg))
            {
                if ((instruction_reg >> 25) != 0)
                    illegal_inst = true;
                else
                    x[RD(instruction_reg)] = (x[RS1(instruction_reg)] << (instruction_reg >> 20));
            }
            else if (IS_SLT(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] < x[RS2(instruction_reg)];
            }
            else if (IS_SLTI(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] < S_I_IMM(instruction_reg);
            }
            else if (IS_SLTIU(instruction_reg))
            {
                x[RD(instruction_reg)] = unsigned(x[RS1(instruction_reg)]) < unsigned(S_I_IMM(instruction_reg));
            }
            else if (IS_SLTU(instruction_reg))
            {
                x[RD(instruction_reg)] = unsigned(x[RS1(instruction_reg)]) < unsigned(x[RS2(instruction_reg)]);
            }
            else if (IS_SRA(instruction_reg))
            {
                x[RD(instruction_reg)] = (x[RS1(instruction_reg)] >> unsigned(x[RS2(instruction_reg)]));
            }
            else if (IS_SRAI(instruction_reg))
            {
                if ((instruction_reg >> 25) != 0)
                    illegal_inst = true;
                else
                    x[RD(instruction_reg)] = (x[RS1(instruction_reg)] >> (instruction_reg >> 20));
            }
            else if (IS_SRL(instruction_reg))
            {
                x[RD(instruction_reg)] = (unsigned(x[RS1(instruction_reg)]) >> unsigned(x[RS2(instruction_reg)]));
            }
            else if (IS_SRLI(instruction_reg))
            {
                if ((instruction_reg >> 25) != 0)
                    illegal_inst = true;
                else
                    x[RD(instruction_reg)] = (unsigned(x[RS1(instruction_reg)]) >> unsigned(instruction_reg >> 20));
            }
            else if (IS_SUB(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] - x[RS2(instruction_reg)];
            }
            else if (IS_XOR(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] ^ x[RS2(instruction_reg)];
            }
            else if (IS_XORI(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] ^ S_I_IMM(instruction_reg);
            }
            else if (IS_MRET(instruction_reg))
            {
                program_count = csr[A_MEPC];
                csr[A_MSTATUS] |= MIE(1);
                csr[A_MSTATUS] |= MPIE(1);
            }
            else if (IS_WFI(instruction_reg))
            {
                // FL模型中实现为nop
            }
            else if (IS_MUL(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] * x[RS2(instruction_reg)];
            }
            else if (IS_MULH(instruction_reg))
            {
                x[RD(instruction_reg)] = ((ll(x[RS1(instruction_reg)]) * ll(x[RS2(instruction_reg)])) >> 32);
            }
            else if (IS_MULHU(instruction_reg))
            {
                x[RD(instruction_reg)] = ((ull(x[RS1(instruction_reg)]) * ull(x[RS2(instruction_reg)])) >> 32);
            }
            else if (IS_MULHSU(instruction_reg))
            {
                x[RD(instruction_reg)] = ((ll(x[RS1(instruction_reg)]) * ull(x[RS2(instruction_reg)])) >> 32);
            }
            else if (IS_DIV(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] / x[RS2(instruction_reg)];
            }
            else if (IS_DIVU(instruction_reg))
            {
                x[RD(instruction_reg)] = unsigned(x[RS1(instruction_reg)]) / unsigned(x[RS2(instruction_reg)]);
            }
            else if (IS_REM(instruction_reg))
            {
                x[RD(instruction_reg)] = x[RS1(instruction_reg)] % x[RS2(instruction_reg)];
            }
            else if (IS_REMU(instruction_reg))
            {
                x[RD(instruction_reg)] = unsigned(unsigned(x[RS1(instruction_reg)]) % unsigned(x[RS2(instruction_reg)]));
            }
            else if (IS_FADD_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fr[RS1(instruction_reg)].dou + fr[RS2(instruction_reg)].dou;
            }
            else if (IS_FADD_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fr[RS1(instruction_reg)].flo + fr[RS2(instruction_reg)].flo;
            }
            else if (IS_FCLASS_S(instruction_reg))
            {
                if (fr[RS1(instruction_reg)].flo == std::numeric_limits<float>::quiet_NaN())
                {
                    x[RD(instruction_reg)] |= (1 << 9);
                }
                if (fr[RS1(instruction_reg)].flo == std::numeric_limits<float>::signaling_NaN())
                {
                    x[RD(instruction_reg)] |= (1 << 8);
                }
                if (fr[RS1(instruction_reg)].flo == std::numeric_limits<float>::infinity())
                {
                    x[RD(instruction_reg)] |= (1 << 7);
                }
                if (frexpf(fr[RS1(instruction_reg)].flo, x) < 1 and frexpf(fr[RS1(instruction_reg)].flo, x) > 0.5 and fr[RS1(instruction_reg)].flo > 0) //判断是否规格化
                {
                    x[RD(instruction_reg)] |= (1 << 6);
                }
                if (!(frexpf(fr[RS1(instruction_reg)].flo, x) < 1 and frexpf(fr[RS1(instruction_reg)].flo, x) > 0.5) and fr[RS1(instruction_reg)].flo > 0) //判断是否规格化
                {
                    x[RD(instruction_reg)] |= (1 << 5);
                }
                if (*(reinterpret_cast<unsigned *>(&fr[RS1(instruction_reg)].flo)) == 0x00000000) //+0
                {
                    x[RD(instruction_reg)] |= (1 << 4);
                }
                if (*(reinterpret_cast<unsigned *>(&fr[RS1(instruction_reg)].flo)) == 0x80000000) //负0
                {
                    x[RD(instruction_reg)] |= (1 << 3);
                }
                if (!(frexpf(fr[RS1(instruction_reg)].flo, x) < 1 and frexpf(fr[RS1(instruction_reg)].flo, x) > 0.5) and fr[RS1(instruction_reg)].flo < 0) //负的非规格化数
                {
                    x[RD(instruction_reg)] |= (1 << 2);
                }
                if (frexpf(fr[RS1(instruction_reg)].flo, x) < 1 and frexpf(fr[RS1(instruction_reg)].flo, x) > 0.5 and fr[RS1(instruction_reg)].flo < 0) //负的规格化数
                {
                    x[RD(instruction_reg)] |= (1 << 1);
                }
                if (fr[RS1(instruction_reg)].flo == -std::numeric_limits<float>::infinity())
                {
                    x[RD(instruction_reg)] |= (1);
                }
            }
            else if (IS_FCLASS_D(instruction_reg))
            {
                if (fr[RS1(instruction_reg)].dou == std::numeric_limits<double>::quiet_NaN())
                {
                    x[RD(instruction_reg)] |= (1 << 9);
                }
                if (fr[RS1(instruction_reg)].dou == std::numeric_limits<double>::signaling_NaN())
                {
                    x[RD(instruction_reg)] |= (1 << 8);
                }
                if (fr[RS1(instruction_reg)].dou == std::numeric_limits<double>::infinity())
                {
                    x[RD(instruction_reg)] |= (1 << 7);
                }
                if (frexp(fr[RS1(instruction_reg)].dou, x) < 1 and frexp(fr[RS1(instruction_reg)].dou, x) > 0.5 and fr[RS1(instruction_reg)].dou > 0) //判断是否规格化
                {
                    x[RD(instruction_reg)] |= (1 << 6);
                }
                if (!(frexp(fr[RS1(instruction_reg)].dou, x) < 1 and frexp(fr[RS1(instruction_reg)].dou, x) > 0.5) and fr[RS1(instruction_reg)].dou > 0) //判断是否规格化
                {
                    x[RD(instruction_reg)] |= (1 << 5);
                }
                if (*(reinterpret_cast<ull *>(&fr[RS1(instruction_reg)].dou)) == 0x0000000000000000) //+0
                {
                    x[RD(instruction_reg)] |= (1 << 4);
                }
                if (*(reinterpret_cast<ull *>(&fr[RS1(instruction_reg)].flo)) == 0x8000000000000000) //负0
                {
                    x[RD(instruction_reg)] |= (1 << 3);
                }
                if (!(frexp(fr[RS1(instruction_reg)].dou, x) < 1 and frexp(fr[RS1(instruction_reg)].dou, x) > 0.5) and fr[RS1(instruction_reg)].dou < 0) //负的非规格化数
                {
                    x[RD(instruction_reg)] |= (1 << 2);
                }
                if (frexp(fr[RS1(instruction_reg)].dou, x) < 1 and frexp(fr[RS1(instruction_reg)].dou, x) > 0.5 and fr[RS1(instruction_reg)].dou < 0) //负的规格化数
                {
                    x[RD(instruction_reg)] |= (1 << 1);
                }
                if (fr[RS1(instruction_reg)].dou == -std::numeric_limits<double>::infinity())
                {
                    x[RD(instruction_reg)] |= (1);
                }
            }
            else if (IS_FCVT_D_S(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fr[RS1(instruction_reg)].flo;
            }
            else if (IS_FCVT_D_W(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = x[RS1(instruction_reg)];
            }
            else if (IS_FCVT_D_WU(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = *(reinterpret_cast<unsigned *>(&x[RS1(instruction_reg)]));
            }
            else if (IS_FCVT_S_D(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fr[RS1(instruction_reg)].dou;
            }
            else if (IS_FCVT_S_W(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = x[RS1(instruction_reg)];
            }
            else if (IS_FCVT_S_WU(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = *(reinterpret_cast<unsigned *>(&x[RS1(instruction_reg)]));
            }
            else if (IS_FCVT_W_D(instruction_reg))
            {
                x[RD(instruction_reg)] = static_cast<int>(fr[RS1(instruction_reg)].dou);
            }
            else if (IS_FCVT_WU_D(instruction_reg))
            {
                x[RD(instruction_reg)] = static_cast<unsigned>(fr[RS1(instruction_reg)].dou);
            }
            else if (IS_FCVT_W_S(instruction_reg))
            {
                x[RD(instruction_reg)] = fr[RS1(instruction_reg)].flo;
            }
            else if (IS_FCVT_WU_S(instruction_reg))
            {
                x[RD(instruction_reg)] = static_cast<unsigned>(fr[RS1(instruction_reg)].flo);
            }
            else if (IS_FDIV_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fr[RS1(instruction_reg)].dou / fr[RS2(instruction_reg)].dou;
            }
            else if (IS_FDIV_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fr[RS1(instruction_reg)].flo / fr[RS2(instruction_reg)].flo;
            }
            else if (IS_FEQ_D(instruction_reg))
            {
                x[RD(instruction_reg)] = fr[RS1(instruction_reg)].dou == fr[RS2(instruction_reg)].dou ? 1 : 0;
            }
            else if (IS_FEQ_S(instruction_reg))
            {
                x[RD(instruction_reg)] = fr[RS1(instruction_reg)].flo == fr[RS2(instruction_reg)].flo ? 1 : 0;
            }
            else if (IS_FLD(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = *(reinterpret_cast<double *>(&mem[x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)]));
            }
            else if (IS_FLE_D(instruction_reg))
            {
                x[RD(instruction_reg)] = fr[RS1(instruction_reg)].dou == fr[RS2(instruction_reg)].dou ? 1 : 0;
            }
            else if (IS_FLE_S(instruction_reg))
            {
                x[RD(instruction_reg)] = fr[RS1(instruction_reg)].flo <= fr[RS2(instruction_reg)].flo ? 1 : 0;
            }
            else if (IS_FLT_D(instruction_reg))
            {
                x[RD(instruction_reg)] = fr[RS1(instruction_reg)].dou < fr[RS2(instruction_reg)].dou ? 1 : 0;
            }
            else if (IS_FLT_S(instruction_reg))
            {
                x[RD(instruction_reg)] = fr[RS1(instruction_reg)].flo < fr[RS2(instruction_reg)].flo ? 1 : 0;
            }
            else if (IS_FLW(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = *(reinterpret_cast<float *>(&mem[x[RS1(instruction_reg)] + S_I_IMM(instruction_reg)]));
            }
            else if (IS_FMADD_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fr[RS1(instruction_reg)].flo * fr[RS2(instruction_reg)].flo + fr[RS3(instruction_reg)].flo;
            }
            else if (IS_FMADD_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fr[RS1(instruction_reg)].dou * fr[RS2(instruction_reg)].dou + fr[RS3(instruction_reg)].dou;
            }
            else if (IS_FMAX_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fmax(fr[RS1(instruction_reg)].dou, fr[RS2(instruction_reg)].dou);
            }
            else if (IS_FMAX_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fmax(fr[RS1(instruction_reg)].flo, fr[RS2(instruction_reg)].flo);
            }
            else if (IS_FMIN_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fmin(fr[RS1(instruction_reg)].dou, fr[RS2(instruction_reg)].dou);
            }
            else if (IS_FMIN_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fmin(fr[RS1(instruction_reg)].flo, fr[RS2(instruction_reg)].flo);
            }
            else if (IS_FMSUB_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fr[RS1(instruction_reg)].dou * fr[RS2(instruction_reg)].dou - fr[RS3(instruction_reg)].dou;
            }
            else if (IS_FMSUB_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fr[RS1(instruction_reg)].flo * fr[RS2(instruction_reg)].flo - fr[RS3(instruction_reg)].flo;
            }
            else if (IS_FMUL_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fr[RS1(instruction_reg)].dou * fr[RS2(instruction_reg)].dou;
            }
            else if (IS_FMUL_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fr[RS1(instruction_reg)].flo * fr[RS2(instruction_reg)].flo;
            }
            else if (IS_FMV_X_W(instruction_reg))
            {
                x[RD(instruction_reg)] = *(reinterpret_cast<int *>(&fr[RS1(instruction_reg)].flo));
            }
            else if (IS_FNMADD_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = -fr[RS1(instruction_reg)].dou * fr[RS2(instruction_reg)].dou + fr[RS3(instruction_reg)].dou;
            }
            else if (IS_FNMADD_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = -fr[RS1(instruction_reg)].flo * fr[RS2(instruction_reg)].flo + fr[RS3(instruction_reg)].flo;
            }
            else if (IS_FNMSUB_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = -fr[RS1(instruction_reg)].dou * fr[RS2(instruction_reg)].dou - fr[RS3(instruction_reg)].dou;
            }
            else if (IS_FNMSUB_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = -fr[RS1(instruction_reg)].flo * fr[RS2(instruction_reg)].flo - fr[RS3(instruction_reg)].flo;
            }
            else if (IS_FSD(instruction_reg))
            {
                double *dp{reinterpret_cast<double *>(&mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)])};
                *dp = fr[RS2(instruction_reg)].dou;
            }
            else if (IS_FSGNJ_D(instruction_reg))
            {
                ll tmp{(*(reinterpret_cast<ll *>(&fr[RS2(instruction_reg)].dou)) & (1ll << 63)) | (*(reinterpret_cast<ll *>(&fr[RS1(instruction_reg)].dou)) & (~(1ll << 63)))};
                fr[RD(instruction_reg)].dou = *(reinterpret_cast<double *>(&tmp));
            }
            else if (IS_FSGNJ_S(instruction_reg))
            {
                int tmp{(*(reinterpret_cast<int *>(&fr[RS2(instruction_reg)].flo)) & (1 << 31)) | (*(reinterpret_cast<int *>(&fr[RS1(instruction_reg)].flo)) & (~(1 << 31)))};
                fr[RD(instruction_reg)].flo = *(reinterpret_cast<float *>(&tmp));
            }
            else if (IS_FSGNJN_D(instruction_reg))
            {

                ll tmp{(~*(reinterpret_cast<ll *>(&(fr[RS2(instruction_reg)].dou))) & (1ll << 63)) | (*reinterpret_cast<ll *>(&fr[RS1(instruction_reg)].dou) & (~(1ll << 63)))};
                fr[RD(instruction_reg)].dou = *(reinterpret_cast<double *>(&tmp));
            }
            else if (IS_FSGNJN_S(instruction_reg))
            {
                int tmp{(~*(reinterpret_cast<int *>(&(fr[RS2(instruction_reg)].flo))) & (1 << 31)) | (*(reinterpret_cast<int *>(&fr[RS1(instruction_reg)].flo)) & (~(1 << 31)))};
                fr[RD(instruction_reg)].flo = *(reinterpret_cast<float *>(&tmp));
            }
            else if (IS_FSGNJX_D(instruction_reg))
            {
                ll tmp{((*(reinterpret_cast<ll *>(&fr[RS1(instruction_reg)].dou)) & (1ll << 63)) ^ (*(reinterpret_cast<ll *>(&fr[RS2(instruction_reg)].dou)) & (1ll << 63))) | (*(reinterpret_cast<ll *>(&fr[RS1(instruction_reg)].dou)) & ~(1ll << 63))};
                fr[RD(instruction_reg)].dou = *(reinterpret_cast<double *>(&tmp));
            }
            else if (IS_FSGNJX_S(instruction_reg))
            {
                int tmp{((*(reinterpret_cast<int *>(&fr[RS1(instruction_reg)].flo)) & (1 << 31)) ^ (*(reinterpret_cast<int *>(&fr[RS2(instruction_reg)].flo)) & (1 << 31))) | (*(reinterpret_cast<int *>(&fr[RS1(instruction_reg)].flo)) & ~(1 << 31))};
                fr[RD(instruction_reg)].flo = *(reinterpret_cast<float *>(&tmp));
            }
            else if (IS_FSQRT_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = sqrtl(fr[RS1(instruction_reg)].dou);
            }
            else if (IS_FSQRT_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = sqrtf(fr[RS1(instruction_reg)].flo);
            }
            else if (IS_FSUB_D(instruction_reg))
            {
                fr[RD(instruction_reg)].dou = fr[RS1(instruction_reg)].dou - fr[RS2(instruction_reg)].dou;
            }
            else if (IS_FSUB_S(instruction_reg))
            {
                fr[RD(instruction_reg)].flo = fr[RS1(instruction_reg)].flo - fr[RS2(instruction_reg)].flo;
            }
            else if (IS_FSW(instruction_reg))
            {
                float *fp{reinterpret_cast<float *>(&mem[x[RS1(instruction_reg)] + S_S_IMM(instruction_reg)])};
                *fp = fr[RS2(instruction_reg)].flo;
            }
            else // 触发不合法指令异常
            {
                illegal_inst = true;
                FLUSH("PC=0x%x, Illegal instruction!\n", program_count);
            }
        }
        catch (const unsigned int e_addr)
        {
            csr[A_MINSTRET]--;                 // 成功执行指令数减一
            csr[A_MCAUSE] = LOAD_ACCESS_FAULT; // 设置异常原因:数据加载虚拟地址异常
            csr[A_MIP] |= MEXCEPTIONIP(1);     // 异常挂起
            csr[A_MTVAL] = e_addr;             // 保存页面虚拟地址
        }
        

        x[0] = 0; //riscv正确性保证
        if (illegal_inst)
        {

            csr[A_MINSTRET]--;                   // 成功执行指令数减一
            csr[A_MCAUSE] = ILLEGAL_INSTRUCTION; // 设置异常原因
            csr[A_MIP] |= MEXCEPTIONIP(1);       // 异常挂起
            csr[A_MTVAL] = instruction_reg;      // 保存指令编码
            illegal_inst = false;                //恢复指令
        }
        if (time_counter[A_MTIME] < time_counter[A_MTIMECMP])
            time_counter[A_MTIME]++;
        else
        {
            //定时器中断
            csr[A_MIP] |= MTIP(1); //定时器中断挂起
        }

        /*中断处理程序应该保存所有寄存器，x18（a2）寄存器用于传递软中断号*/
        // 中断判断与处理
        if ((csr[A_MSTATUS] & MIE(1)) != 0 and csr[A_MIP] != 0) //中断使能并且有中断时
        {

            //禁止中断嵌套：按中断优先级依次处理异常，外部中断，定时器中断，软件中断。mret时自动恢复中断使能。
            if ((csr[A_MIP] & MEXCEPTIONIP(1)) != 0)
            {

                csr[A_MSTATUS] &= ~MIE(1);       //关全局中断
                csr[A_MSTATUS] |= MPIE(1);       //保存先前的全局中断状态
                csr[A_MEPC] = program_count - 4; // 虽然此处异常不会返回，但仍然标明当前指令地址，指令运行不正确必须重新运行

                //判断模式
                if ((csr[A_MTVEC] & 0b01) == 0)
                {

                    //均跳转到mtvec处
                    program_count = (csr[A_MTVEC] & ~0b11);
                }
                else
                {
                    //按地址跳转
                    program_count = (csr[A_MTVEC] & ~0b11) + ((csr[A_MCAUSE] & 0x7fffffff) << 2);
                }
            }
            else if ((csr[A_MIP] & MEIP(1)) != 0 and (csr[A_MIE] & MEIE(1)) != 0)
            {

                csr[A_MSTATUS] &= ~MIE(1);                  //关全局中断
                csr[A_MSTATUS] |= MPIE(1);                  //保存先前的全局中断状态
                csr[A_MCAUSE] = MACHINE_EXTERNAL_INTERRUPT; // 设置外部中断
                csr[A_MEPC] = program_count;                // 设置为下一条指令的地址
                
                //判断模式
                if ((csr[A_MTVEC] & 0b01) == 0)
                {
                    //均跳转到mtvec处
                    program_count = (csr[A_MTVEC] & ~0b11);
                }
                else
                {
                    //按地址跳转
                    program_count = (csr[A_MTVEC] & ~0b11) + ((csr[A_MCAUSE] & 0x7fffffff) << 2);
                }
            }
            else if ((csr[A_MIP] & MTIP(1)) != 0 and (csr[A_MIE] & MEIE(1)) != 0)
            {
                csr[A_MSTATUS] &= ~MIE(1);               //关全局中断
                csr[A_MSTATUS] |= MPIE(1);               //保存先前的全局中断状态
                csr[A_MCAUSE] = MACHINE_TIMER_INTERRUPT; // 设置定时器中断
                csr[A_MEPC] = program_count;             // 设置为下一条指令的地址
                //判断模式
                if ((csr[A_MTVEC] & 0b01) == 0)
                {
                    //均跳转到mtvec处
                    program_count = (csr[A_MTVEC] & ~0b11);
                }
                else
                {
                    //按地址跳转
                    program_count = (csr[A_MTVEC] & ~0b11) + ((csr[A_MCAUSE] & 0x7fffffff) << 2);
                }
            }
            else if ((csr[A_MIP] & MSIP(1)) != 0 and (csr[A_MIE] & MSIE(1)) != 0)
            {

                csr[A_MSTATUS] &= ~MIE(1);                  //关全局中断
                csr[A_MSTATUS] |= MPIE(1);                  //保存先前的全局中断状态
                csr[A_MCAUSE] = MACHINE_SOFTWARE_INTERRUPT; // 设置软件中断
                csr[A_MEPC] = program_count;                // 设置为下一条指令的地址
                //判断模式
                if ((csr[A_MTVEC] & 0b01) == 0)
                {
                    //均跳转到mtvec处
                    program_count = (csr[A_MTVEC] & ~0b11);
                }
                else
                {
                    //按地址跳转
                    program_count = (csr[A_MTVEC] & ~0b11) + ((csr[A_MCAUSE] & 0x7fffffff) << 2);
                }
            }
        }
        //编程时不到万不得已不要禁止全局中断，否则异常被忽略.中断禁止时不要产生软中断。
        else if ((csr[A_MIP] & MEXCEPTIONIP(1)) != 0)
        { //在中断处理过程或者禁止中断过程中发生的所有异常均被忽略，容错性设计。防止中断开启后异常处理错误。
            csr[A_MIP] &= ~MEXCEPTIONIP(1);
        }

        if (time_counter[A_MTIME] < time_counter[A_MTIMECMP])
            time_counter[A_MTIME]++;
        else
        {
            //定时器中断
            csr[A_MIP] |= MTIP(1); //定时器中断挂起
        }
        csr[A_MINSTRET]++;
    }
}
//模拟BIOS的ROM:载入磁盘前4KB到RAM中
bool read_to_mem(char path[])
{
    if (disk.setpath(path) == false)
    {
        fprintf(stderr, "磁盘文件找不到或者不能打开！\n");
        return false;
    }

    FILE *fp = disk.getFp();
    int i{};

    //二进制文件不能使用EOF,rom从硬盘上读入4kb
    while (!feof(fp) and i < (1 << 10))
    {
        fread(&mem[i << 2], sizeof(int), 1, fp);
        i++;
    }
    return true;
}
// 键盘模拟器
void *tty(void *_)
{
    while (true)
    {
        board.boardbuf = getchar();
        csr[A_MIP] |= MEIP(1);
        sleep(0.2);
    }
    return nullptr;
}
void sighandle(int sig)
{
    tcsetattr(0, TCSANOW, &origin_settings);
    char ch{-1};
    FLUSH("模拟器结束运行。\n");
    if (sig == SIGSEGV)
        FLUSH("内存访问错误！\n");
    send(client_sockfd, &ch, 1, 0);
    close(client_sockfd);
    close(server_sockfd);
    if (disk.has_disk)
        disk.close();
    printf("\n");
    exit(0);
}
int main(int argc, char *argv[])
{
    tcgetattr(0, &origin_settings);
    signal(SIGSEGV, sighandle);
    signal(SIGINT, sighandle);
    signal(SIGHUP, sighandle);
    signal(SIGKILL, sighandle);
    signal(SIGABRT, sighandle);
    signal(SIGTSTP, sighandle);
    signal(SIGQUIT, sighandle);
    signal(SIGPIPE, sighandle);
    
    printf("-------------CPU simulator-------------\n");
    printf("| Powered by Chang, Kaiyan of UESTC.   |\n");
    printf("---------------------------------------\n");
    if (argc != 2 or read_to_mem(argv[1]) == false)
    {
        fprintf(stderr, "没有找到磁盘文件!\n");
        return 1;
    }
    socklen_t client_len = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    //创建流套接字
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_sockfd != -1);
    server_addr.sin_family = AF_INET;                //指定网络套接字
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //接受所有IP地址的连接
    server_addr.sin_port = htons(8001);              //绑定到8001端口
    //绑定（命名）套接字
    assert(bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != -1);
    //创建套接字队列，监听套接字
    listen(server_sockfd, 5);
    client_len = sizeof(client_addr);
    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &client_len);
    printf("监视器连接成功！\n");
    tcgetattr(0, &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= (~ECHO);
    stored_settings = new_settings;
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);
    pthread_t th;
    system("clear");
    pthread_create(&th, NULL, tty, NULL);
    cpu_start();

    if (pthread_cancel(th) != 0)
    {
        FLUSH("Error in exit the thread!\n");
    }
    pthread_join(th, 0);
    tcsetattr(0, TCSANOW, &origin_settings);
    char ch{-1};
    send(client_sockfd, &ch, 1, 0);
    if (disk.has_disk)
        disk.close();
    close(client_sockfd);
    close(server_sockfd);
    printf("\n");
    return 0;
}
//Segmentation fault (core dumped)->存储器超出界限