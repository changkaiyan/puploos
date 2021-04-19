#pragma once
#define get_ppn(x) (reinterpret_cast<unsigned>(x)>>12) // 得到页面4kb对齐地址号

#define disable_irq()                          \
    do                                         \
    {                                          \
        asm volatile("csrci mstatus, (1<<3)"); \
    } while (0)
#define enable_irq()                           \
    do                                         \
    {                                          \
        asm volatile("csrsi mstatus, (1<<3)"); \
    } while (0)
#define enable_page()                        \
    do                                       \
    {                                        \
        asm volatile("csrsi satp, (1<<31)"); \
    } while (0)
#define ebreak()                \
    do                          \
    {                           \
        asm volatile("ebreak"); \
    } while (0)
#define read_csr(reg) ({          \
    unsigned ret;                 \
    asm volatile("csrr %0, " #reg \
                 : "=r"(ret));    \
    ret;                          \
})
#define write_csr(reg, dat)              \
    do                                   \
    {                                    \
        unsigned data = dat;             \
        asm volatile("csrw " #reg ", %0" \
                     :                   \
                     : "r"(data));       \
    } while (0)
#define page_item(ppn, v, r, w, x) ((ppn << 10) | (v & 1) | ((r << 1) & 0b10) | ((w << 2) & 0b100) | ((x << 3) & 0b1000))