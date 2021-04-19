/**
 * 文档代码化：RV32I指令集中用到的常数及指令译码宏
 */
#pragma once
using ll = long long;
using llp = long long *;
using ull = unsigned long long;
#define FLUSH(...)                    \
    do                                \
    {                                 \
        flush_to_socket(__VA_ARGS__); \
    } while (false)
#define MEM_KB(x) (x << 8)
#define MEM_MB(x) (MEM_KB(x) << 10)
#define TRANSMEM(x) ((unsigned(x)) >> 2)
// 指令译码提取宏
#define OPCODE(x) (x & 0b1111111)
#define RD(x) ((x >> 7) & 0b11111)
#define RS1(x) ((x >> 15) & 0b11111)
#define RS2(x) ((x >> 20) & 0b11111)
#define RS3(x) ((x >> 27) & 0b11111)
#define FUNCT3(x) ((x >> 12) & 0b111)
#define FUNCT7(x) ((x >> 25) & 0b1111111)
// 下面是无符号位扩展的立即数,默认x为unsigned
#define R_IMM(x) (FUNCT7(x))
#define I_IMM(x) ((x >> 20) & 0b111'111'111'111)
#define S_IMM(x) (((x >> 7) & 0b11111) | ((x >> 20) & 0b111'111'100'000))
#define B_IMM(x) (((x >> 7) & 0b11110) |            \
                  ((x << 4) & 0b100'000'000'000) |  \
                  ((x >> 20) & 0b111'111'100'000) | \
                  ((x >> 19) & 0b1'000'000'000'000))
#define U_IMM(x) (x & 0xfffff000)
#define J_IMM(x) ((x & 0xff000) |                  \
                  ((x >> 9) & 0b100'000'000'000) | \
                  ((x >> 20) & 0b11'111'111'110) | \
                  ((x >> 11) & 0x100000))

// 下面是有符号位扩展的立即数,need to check
#define S_R_IMM(x) (int(x) >> 25)
#define S_I_IMM(x) int(int(x) >> 20)
#define S_S_IMM(x) int(((x >> 7) & 0b11111) | \
                       ((int(x) >> 20) & 0xffffffe0))
#define S_B_IMM(x) int(((x >> 7) & 0b11110) |            \
                       ((x << 4) & 0b100'000'000'000) |  \
                       ((x >> 20) & 0b111'111'100'000) | \
                       (unsigned((int(x) >> 19) & 0xfffff000)))
#define S_U_IMM(x) int(x & 0xfffff000)
#define S_J_IMM(x) int((x & 0xff000) |                  \
                       ((x >> 9) & 0b100'000'000'000) | \
                       ((x >> 20) & 0b11'111'111'110) | \
                       (unsigned(int(x) >> 11) & 0xfff00000))
// 指令分类判断宏
#define R_TYPE(x) (OPCODE(x) == 0b0110011)
#define S_TYPE(x) (OPCODE(x) == 0b0100011)
#define B_TYPE(x) (OPCODE(x) == 0b1100011)
#define U_TYPE(x) (OPCODE(x) == 0b0110111 or \
                   OPCODE(x) == 0b0010111)
#define I_TYPE(x) (OPCODE(x) == 0b1100111 or \
                   OPCODE(x) == 0b0000011 or \
                   OPCODE(x) == 0b0010011 or \
                   OPCODE(x) == 0b1110011 or \
                   OPCODE(x) == 0b0001111)
#define J_TYPE(x) (OPCODE(x) == 0b1101111)

// 具体指令判断宏

#define IS_LUI(x) (OPCODE(x) == 0b0110111)
#define IS_AUIPC(x) (OPCODE(x) == 0b0010111)
#define IS_JAL(x) (OPCODE(x) == 0b1101111)
#define IS_JALR(x) (OPCODE(x) == 0b1100111 and \
                    FUNCT3(x) == 0b000)
#define IS_BEQ(x) (OPCODE(x) == 0b1100011 and \
                   FUNCT3(x) == 0b000)
#define IS_BNE(x) (OPCODE(x) == 0b1100011 and \
                   FUNCT3(x) == 0b001)
#define IS_BLT(x) (OPCODE(x) == 0b1100011 and \
                   FUNCT3(x) == 0b100)
#define IS_BGE(x) (OPCODE(x) == 0b1100011 and \
                   FUNCT3(x) == 0b101)
#define IS_BLTU(x) (OPCODE(x) == 0b1100011 and \
                    FUNCT3(x) == 0b110)
#define IS_BGEU(x) (OPCODE(x) == 0b1100011 and \
                    FUNCT3(x) == 0b111)
#define IS_LB(x) (OPCODE(x) == 0b0000011 and \
                  FUNCT3(x) == 0b000)
#define IS_LH(x) (OPCODE(x) == 0b0000011 and \
                  FUNCT3(x) == 0b001)
#define IS_LW(x) (OPCODE(x) == 0b0000011 and \
                  FUNCT3(x) == 0b010)
#define IS_LBU(x) (OPCODE(x) == 0b0000011 and \
                   FUNCT3(x) == 0b100)
#define IS_LHU(x) (OPCODE(x) == 0b0000011 and \
                   FUNCT3(x) == 0b101)
#define IS_SB(x) (OPCODE(x) == 0b0100011 and \
                  FUNCT3(x) == 0b000)
#define IS_SH(x) (OPCODE(x) == 0b0100011 and \
                  FUNCT3(x) == 0b001)
#define IS_SW(x) (OPCODE(x) == 0b0100011 and \
                  FUNCT3(x) == 0b010)
#define IS_ADDI(x) (OPCODE(x) == 0b0010011 and \
                    FUNCT3(x) == 0b000)
#define IS_SLTI(x) (OPCODE(x) == 0b0010011 and \
                    FUNCT3(x) == 0b010)
#define IS_SLTIU(x) (OPCODE(x) == 0b0010011 and \
                     FUNCT3(x) == 0b011)
#define IS_XORI(x) (OPCODE(x) == 0b0010011 and \
                    FUNCT3(x) == 0b100)
#define IS_ORI(x) (OPCODE(x) == 0b0010011 and \
                   FUNCT3(x) == 0b110)
#define IS_ANDI(x) (OPCODE(x) == 0b0010011 and \
                    FUNCT3(x) == 0b111)
#define IS_SLLI(x) (OPCODE(x) == 0b0010011 and \
                    FUNCT3(x) == 0b001 and     \
                    FUNCT7(x) == 0b0000000)
#define IS_SRLI(x) (OPCODE(x) == 0b0010011 and \
                    FUNCT3(x) == 0b101 and     \
                    FUNCT7(x) == 0b0000000)
#define IS_SRAI(x) (OPCODE(x) == 0b0010011 and \
                    FUNCT3(x) == 0b101 and     \
                    FUNCT7(x) == 0b0100000)
#define IS_ADD(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b000 and     \
                   FUNCT7(x) == 0b0000000)
#define IS_SUB(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b000 and     \
                   FUNCT7(x) == 0b0100000)
#define IS_SLL(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b001 and     \
                   FUNCT7(x) == 0b0000000)
#define IS_SLT(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b010 and     \
                   FUNCT7(x) == 0b0000000)
#define IS_SLTU(x) (OPCODE(x) == 0b0110011 and \
                    FUNCT3(x) == 0b011 and     \
                    FUNCT7(x) == 0b0000000)
#define IS_XOR(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b100 and     \
                   FUNCT7(x) == 0b0000000)
#define IS_SRL(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b101 and     \
                   FUNCT7(x) == 0b0000000)
#define IS_SRA(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b101 and     \
                   FUNCT7(x) == 0b0100000)
#define IS_OR(x) (OPCODE(x) == 0b0110011 and \
                  FUNCT3(x) == 0b110 and     \
                  FUNCT7(x) == 0b0000000)
#define IS_AND(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b111 and     \
                   FUNCT7(x) == 0b0000000)
#define IS_FENCE(x) (OPCODE(x) == 0b0001111 and \
                     FUNCT3(x) == 0b000)
#define IS_FENCEI(x) (OPCODE(x) == 0b0001111 and \
                      FUNCT3(x) == 0b001)
#define IS_ECALL(x) (OPCODE(x) == 0b1110011 and     \
                     I_IMM(x) == 0b000000000000 and \
                     RS1(x) == 0 and                \
                     RD(x) == 0 and                 \
                     FUNCT3(x) == 0)
// ebreak的risc-v的文档中有指令表述不一致，此处已纠正
#define IS_EBREAK(x) (OPCODE(x) == 0b1110011 and \
                      I_IMM(x) == 0b000000000001)
#define IS_CSRRW(x) (OPCODE(x) == 0b1110011 and \
                     FUNCT3(x) == 0b001)
#define IS_CSRRS(x) (OPCODE(x) == 0b1110011 and \
                     FUNCT3(x) == 0b010)
#define IS_CSRRC(x) (OPCODE(x) == 0b1110011 and \
                     FUNCT3(x) == 0b011)
#define IS_CSRRWI(x) (OPCODE(x) == 0b1110011 and \
                      FUNCT3(x) == 0b101)
#define IS_CSRRSI(x) (OPCODE(x) == 0b1110011 and \
                      FUNCT3(x) == 0b110)
#define IS_CSRRCI(x) (OPCODE(x) == 0b1110011 and \
                      FUNCT3(x) == 0b111)
#define IS_MRET(x) (x == 0x30200073)
#define IS_WFI(x) (x == 0x10500073)
/**
 * --------------------------------------------------------------
 * 
 * RV32MAFD指令集与CSR寄存器定义区
 * 
 * 
 * ----------------------------------------------------------------
 */
#define IS_MUL(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b000 and     \
                   FUNCT7(x) == 0b0000001)
#define IS_MULH(x) (OPCODE(x) == 0b0110011 and \
                    FUNCT3(x) == 0b001 and     \
                    FUNCT7(x) == 0b0000001)
#define IS_MULHSU(x) (OPCODE(x) == 0b0110011 and \
                      FUNCT3(x) == 0b010 and     \
                      FUNCT7(x) == 0b0000001)
#define IS_MULHU(x) (OPCODE(x) == 0b0110011 and \
                     FUNCT3(x) == 0b011 and     \
                     FUNCT7(x) == 0b0000001)
#define IS_DIV(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b100 and     \
                   FUNCT7(x) == 0b0000001)
#define IS_DIVU(x) (OPCODE(x) == 0b0110011 and \
                    FUNCT3(x) == 0b101 and     \
                    FUNCT7(x) == 0b0000001)
#define IS_REM(x) (OPCODE(x) == 0b0110011 and \
                   FUNCT3(x) == 0b110 and     \
                   FUNCT7(x) == 0b0000001)
#define IS_REMU(x) (OPCODE(x) == 0b0110011 and \
                    FUNCT3(x) == 0b111 and     \
                    FUNCT7(x) == 0b0000001)
//--------------
#define IS_FLW(x) (OPCODE(x) == 0b0000111 and \
                   FUNCT3(x) == 0b010)
#define IS_FSW(x) (OPCODE(x) == 0b0100111 and \
                   FUNCT3(x) == 0b010)
#define IS_FMADD_S(x) (OPCODE(x) == 0b1000011 and \
                       ((x >> 25) & 0b11) == 0b00)
#define IS_FMSUB_S(x) (OPCODE(x) == 0b1000111 and \
                       ((x >> 25) & 0b11) == 0b00)
#define IS_FNMSUB_S(x) (OPCODE(x) == 0b1001011 and \
                        ((x >> 25) & 0b11) == 0b00)
#define IS_FNMADD_S(x) (OPCODE(x) == 0b1001111 and \
                        ((x >> 25) & 0b11) == 0b00)
#define IS_FADD_S(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0000000)
#define IS_FSUB_S(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0000100)
#define IS_FMUL_S(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0001000)
#define IS_FDIV_S(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0001100)
#define IS_FSQRT_S(x) (OPCODE(x) == 0b1010011 and \
                       FUNCT7(x) == 0b0101100 and \
                       ((x >> 20) & 0b11111) == 0b00000)
#define IS_FSGNJ_S(x) (OPCODE(x) == 0b1010011 and \
                       FUNCT7(x) == 0b0010000 and \
                       FUNCT3(x) == 0b000)
#define IS_FSGNJN_S(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b0010000 and \
                        FUNCT3(x) == 0b001)
#define IS_FSGNJX_S(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b0010000 and \
                        FUNCT3(x) == 0b010)
#define IS_FMIN_S(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0010100 and \
                      FUNCT3(x) == 0b000)
#define IS_FMAX_S(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0010100 and \
                      FUNCT3(x) == 0b001)
#define IS_FCVT_W_S(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b1100000 and \
                        ((x >> 20) & 0b11111) == 0b00000)
#define IS_FCVT_WU_S(x) (OPCODE(x) == 0b1010011 and \
                         FUNCT7(x) == 0b1100000 and \
                         ((x >> 20) & 0b11111) == 0b00001)
#define IS_FMV_X_W(x) (OPCODE(x) == 0b1010011 and \
                       FUNCT7(x) == 0b1110000 and \
                       ((x >> 20) & 0b11111) == 0b00000)
#define IS_FEQ_S(x) (OPCODE(x) == 0b1010011 and \
                     FUNCT7(x) == 0b1010000 and \
                     FUNCT3(x) == 0b010)
#define IS_FLT_S(x) (OPCODE(x) == 0b1010011 and \
                     FUNCT7(x) == 0b1010000 and \
                     FUNCT3(x) == 0b001)
#define IS_FLE_S(x) (OPCODE(x) == 0b1010011 and \
                     FUNCT7(x) == 0b1010000 and \
                     FUNCT3(x) == 0b000)
#define IS_FCLASS_S(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b1110000 and \
                        FUNCT3(x) == 0b001 and     \
                        ((x >> 20) & 0b11111) == 0b00000)
#define IS_FCVT_S_W(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b1101000 and \
                        ((x >> 20) & 0b11111) == 0b00000)
#define IS_FCVT_S_WU(x) (OPCODE(x) == 0b1010011 and \
                         FUNCT7(x) == 0b1101000 and \
                         ((x >> 20) & 0b11111) == 0b00001)
#define IS_FMV_W_X(x) (OPCODE(x) == 0b1010011 and \
                       FUNCT7(x) == 0b1111000 and \
                       FUNCT3(x) == 0b000 and     \
                       ((x >> 20) & 0b11111) == 0b00000)
//-------------
#define IS_FLD(x) (OPCODE(x) == 0b0000111 and \
                   FUNCT3(x) == 0b011)
#define IS_FSD(x) (OPCODE(x) == 0b0100111 and \
                   FUNCT3(x) == 0b011)
#define IS_FMADD_D(x) (OPCODE(x) == 0b1000011 and \
                       ((x >> 25) & 0b11) == 0b01)
#define IS_FMSUB_D(x) (OPCODE(x) == 0b1000111 and \
                       ((x >> 25) & 0b11) == 0b01)
#define IS_FNMSUB_D(x) (OPCODE(x) == 0b1001011 and \
                        ((x >> 25) & 0b11) == 0b01)
#define IS_FNMADD_D(x) (OPCODE(x) == 0b1001111 and \
                        ((x >> 25) & 0b11) == 0b01)
#define IS_FADD_D(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0000001)
#define IS_FSUB_D(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0000101)
#define IS_FMUL_D(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0001001)
#define IS_FDIV_D(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0001101)
#define IS_FSQRT_D(x) (OPCODE(x) == 0b1010011 and \
                       FUNCT7(x) == 0b0101101 and \
                       ((x >> 20) & 0b11111) == 0b00000)
#define IS_FSGNJ_D(x) (OPCODE(x) == 0b1010011 and \
                       FUNCT7(x) == 0b0010001 and \
                       FUNCT3(x) == 0b000)
#define IS_FSGNJN_D(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b0010001 and \
                        FUNCT3(x) == 0b001)
#define IS_FSGNJX_D(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b0010001 and \
                        FUNCT3(x) == 0b010)
#define IS_FMIN_D(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0010101 and \
                      FUNCT3(x) == 0b000)
#define IS_FMAX_D(x) (OPCODE(x) == 0b1010011 and \
                      FUNCT7(x) == 0b0010101 and \
                      FUNCT3(x) == 0b001)
#define IS_FCVT_S_D(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b0100000 and \
                        ((x >> 20) & 0b11111) == 0b00001)
#define IS_FCVT_D_S(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b0100001 and \
                        ((x >> 20) & 0b11111) == 0b00000)
#define IS_FEQ_D(x) (OPCODE(x) == 0b1010011 and \
                     FUNCT7(x) == 0b1010001 and \
                     FUNCT3(x) == 0b010)
#define IS_FLT_D(x) (OPCODE(x) == 0b1010011 and \
                     FUNCT7(x) == 0b1010001 and \
                     FUNCT3(x) == 0b001)
#define IS_FLE_D(x) (OPCODE(x) == 0b1010011 and \
                     FUNCT7(x) == 0b1010001 and \
                     FUNCT3(x) == 0b000)
#define IS_FCLASS_D(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT3(x) == 0b001 and     \
                        FUNCT7(x) == 0b1110001 and \
                        ((x >> 20) & 0b11111) == 0b00000)
#define IS_FCVT_W_D(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b1100001 and \
                        ((x >> 20) & 0b11111) == 0b00000)
#define IS_FCVT_WU_D(x) (OPCODE(x) == 0b1010011 and \
                         FUNCT7(x) == 0b1100001 and \
                         ((x >> 20) & 0b11111) == 0b00001)
#define IS_FCVT_D_W(x) (OPCODE(x) == 0b1010011 and \
                        FUNCT7(x) == 0b1101001 and \
                        ((x >> 20) & 0b11111) == 0b00000)
#define IS_FCVT_D_WU(x) (OPCODE(x) == 0b1010011 and \
                         FUNCT7(x) == 0b1101001 and \
                         ((x >> 20) & 0b11111) == 0b00001)
//------------
#define IS_LR_W(x) (OPCODE(x) == 0b0101111 and           \
                    FUNCT3(x) == 0b010 and               \
                    ((x >> 20) & 0b11111) == 0b00000 and \
                    ((x >> 27) & 0b11111) == 0b00010)
#define IS_SC_W(x) (OPCODE(x) == 0b0101111 and \
                    FUNCT3(x) == 0b010 and     \
                    ((x >> 27) & 0b11111) == 0b00011)
#define IS_AMOSWAP_W(x) (OPCODE(x) == 0b0101111 and \
                         FUNCT3(x) == 0b010 and     \
                         ((x >> 27) & 0b11111) == 0b00001)
#define IS_AMOADD_W(x) (OPCODE(x) == 0b0101111 and \
                        FUNCT3(x) == 0b010 and     \
                        ((x >> 27) & 0b11111) == 0b00000)
#define IS_AMOXOR_W(x) (OPCODE(x) == 0b0101111 and \
                        FUNCT3(x) == 0b010 and     \
                        ((x >> 27) & 0b11111) == 0b00100)
#define IS_AMOAND_W(x) (OPCODE(x) == 0b0101111 and \
                        FUNCT3(x) == 0b010 and     \
                        ((x >> 27) & 0b11111) == 0b01100)
#define IS_AMOOR_W(x) (OPCODE(x) == 0b0101111 and \
                       FUNCT3(x) == 0b010 and     \
                       ((x >> 27) & 0b11111) == 0b01000)
#define IS_AMOMIN_W(x) (OPCODE(x) == 0b0101111 and \
                        FUNCT3(x) == 0b010 and     \
                        ((x >> 27) & 0b11111) == 0b10000)
#define IS_AMOMAX_W(x) (OPCODE(x) == 0b0101111 and \
                        FUNCT3(x) == 0b010 and     \
                        ((x >> 27) & 0b11111) == 0b10100)
#define IS_AMOMINU_W(x) (OPCODE(x) == 0b0101111 and \
                         FUNCT3(x) == 0b010 and     \
                         ((x >> 27) & 0b11111) == 0b11000)
#define IS_AMOMAXU_W(x) (OPCODE(x) == 0b0101111 and \
                         FUNCT3(x) == 0b010 and     \
                         ((x >> 27) & 0b11111) == 0b11100)
// 用于计算机操作系统识别硬件信息
#define A_MVENDORID 0xf11 //MRO 厂商id
#define A_MARCHID 0xf12   //MRO 架构id
#define A_MIMPID 0xf13    //MRO 实现id
#define A_MHARTID 0xf14   //MRO 硬线程id
#define A_MINSTRET 0xb02  //MRW 处理器执行指令数
// 时钟周期计数器
#define A_MTIME 0    //time_counter寄存器0
#define A_MTIMECMP 1 //time_counter寄存器1
// 中断与异常处理
#define A_MSTATUS 0x300  //MRW 机器状态寄存器
#define A_MISA 0x301     //MRW 指令集扩展
#define A_MIE 0x304      //MRW 中断使能寄存器
#define A_MTVEC 0x305    //MRW 异常模式入口基地址寄存器
#define A_MEPC 0x341     //MRW 异常pc寄存器
#define A_MCAUSE 0x342   //MRW 异常原因寄存器
#define A_MTVAL 0x343    //MRW 异常地址或指令
#define A_MIP 0x344      //MRW 中断挂起寄存器
#define A_MSCRATCH 0x340 //MRW 单独存放一个暂时值
// 分页与mmu组件
#define A_SATP 0x180 // 机器模式分页组件
//默认值
#define DEFAULT_TIMECMP 0xffffffff                                               //默认时钟中断值
#define REG_NUMBER 32                                                            //通用寄存器数目
#define CSR_NUMBER 0xfff                                                         //csr寄存器数目
#define MTVECMODE 0                                                              //设置mtvec的模式为非向量中断
#define INTERPROGRAM (0xffff00 | MTVECMODE)                                      //中断入口地址和模式
#define VERSION 1                                                                //第一版cpu设计
#define DEFAULT_ISA ((1 << 30) | (1 << 8) | (1 << 12) | (1 << 5) | (1 << 3) | 1) // 默认支持RV32IMAFD指令集
//mcause最高位的编码
#define INTERRUPT (1 << 31) //中断
#define EXCEPTION (0 << 31) //异常
//处理器支持的中断编码
#define MACHINE_SOFTWARE_INTERRUPT (3 | INTERRUPT)  // 软中断
#define MACHINE_TIMER_INTERRUPT (7 | INTERRUPT)     // 时钟中断
#define MACHINE_EXTERNAL_INTERRUPT (11 | INTERRUPT) // 外部中断
// 处理器支持的异常
#define ILLEGAL_INSTRUCTION (2 | EXCEPTION) // 不合法指令异常
#define LOAD_ACCESS_FAULT (5 | EXCEPTION)   // 数据加载异常
//mstatus相关属性
#define MIE(x) (x << 3)  // 中断使能
#define MPIE(x) (x << 7) //设置之前中断使能的信号
//MIE和MIP相关属性
#define MEIE(x) (x << 11)         //外部中断使能
#define MTIE(x) (x << 7)          //定时器中断使能
#define MSIE(x) (x << 3)          //软件中断使能
#define MEIP(x) (x << 11)         //外部中断挂起
#define MTIP(x) (x << 7)          //定时器中断挂起
#define MSIP(x) (x << 3)          //软件中断挂起
#define MEXCEPTIONIP(x) (x << 10) //非标准：异常挂起
//fcs寄存器相关位
#define FCS_NX(x) (x & 1)             //不精确
#define FCS_UF(x) (x << 1)            //下溢
#define FCS_OF(x) (x << 2)            //上溢
#define FCS_DZ(x) (x << 3)            //除以零
#define FCS_NV(x) (x << 4)            //非法操作
#define FCS_FRM(x) ((x & 0b111) << 5) //向最近的偶数舍入
//分页组件
#define SATP_MODE(x) ((x) << 31)                   //分页模式
#define SATP_TO_PPN(x) ((ull(x) & 0x3fffff) << 12) //satp->PPN转换为34位地址
//页表组件
#define GET_PPN(x) ((ull(x) >> 10) << 12)             //pte->二级页表基地址
#define GET_OFFSET(x) ((x)&0xfff)                     //va->偏移
#define GET_VPN1(x) ((ull(x) >> 20) & 0xffc)          //va->第一级页号
#define GET_VPN0(x) ((ull(x) >> 10) & 0xffc)          //va->第二级页号
#define GET_PTE1(x, y) (SATP_TO_PPN(x) | GET_VPN1(y)) //satp,va->一级页表项地址pte1
#define GET_PTE2(x, y) (GET_PPN(x) | GET_VPN0(y))     //pte1,va->二级页表项地址pte2
#define GET_PA(x, y) (GET_PPN(x) | GET_OFFSET(y))     //pte2,va->物理地址