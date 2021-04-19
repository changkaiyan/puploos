#pragma once
#define PAGE_ITEM_NUMBER 1024                                       //一个页表中页表项的数目
#define PAGESIZE 4096                                               //页面大小
#define KERNEL_BEGIN_PPN 0x3000                                     //内核起始页面号
#define PAGENUM 0x4000                                              //页面数
#define VIRTUAL_PAGENUM 0x100000                                    //4GB虚拟地址空间页面数
#define PAGE_MODE (1 << 31)                                         //分页模式
#define MAX_PAGE_NUM 4                                              //单个进程最大可占用一级页表数
#define get_ppn(x) (reinterpret_cast<unsigned>(x) >> 12)            // 得到页面4kb对齐地址号 x：虚拟/物理地址
#define get_ppn_from_pte(x) ((x) >> 10)                             //从页表项得到ppn
#define get_vpn1(x) (reinterpret_cast<unsigned>(x) >> 22)           //从虚拟地址得到vpn1
#define get_vpn0(x) ((reinterpret_cast<unsigned>(x) >> 12) & 0x3ff) //从虚拟地址得到vpn0
#define REGISTER_OBJECT(code)                                                                                   \
    union Object                                                                                                \
    {                                                                                                           \
        Object() {}                                                                                             \
        /*C++联合体构造方法，对象联合体必须要有，因为编译器删除了原生构造方法*/ \
        code                                                                                                    \
    }; //对象类型注册表
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
#define read_csr(reg) ({          \
    unsigned ret;                 \
    asm volatile("csrr %0, " #reg \
                 : "=r"(ret));    \
    ret;                          \
})
#define ebreak()                \
    do                          \
    {                           \
        asm volatile("ebreak"); \
    } while (0)

#define write_csr(reg, dat)              \
    do                                   \
    {                                    \
        unsigned data = dat;             \
        asm volatile("csrw " #reg ", %0" \
                     :                   \
                     : "r"(data));       \
    } while (0)
#define ecall(id, arg1, arg2, arg3)                       \
    do                                                    \
    {                                                     \
        unsigned tmp0 = unsigned(id);   \
        unsigned tmp1 = unsigned(arg1); \
        unsigned tmp2 = unsigned(arg2); \
        unsigned tmp3 = unsigned(arg3); \
        asm volatile("lw a0,%0" ::"m"(tmp0)               \
                     : "a0");                             \
        asm volatile("lw a1,%0" ::"m"(tmp1)               \
                     : "a1");                             \
        asm volatile("lw a2,%0" ::"m"(tmp2)               \
                     : "a2");                             \
        asm volatile("lw a3,%0" ::"m"(tmp3)               \
                     : "a3");                             \
        asm volatile("ecall");                            \
    } while (0)
#define page_item(ppn, v, r, w, x, rsw) (((ppn) << 10) | ((rsw & 0b1) << 8) | (v & 1) | ((r << 1) & 0b10) | ((w << 2) & 0b100) | ((x << 3) & 0b1000)) //rsw为1表示页面在内存或者外存中，0表示页面无效。v=1表示在内存中，0表示页面无或者在外存中
#define get_rsw(pte) ((pte >> 8) & 0b1)
#define INTERRUPT (1 << 31)                         //中断
#define EXCEPTION (0 << 31)                         //异常
#define MACHINE_SOFTWARE_INTERRUPT (3 | INTERRUPT)  //软中断
#define MACHINE_TIMER_INTERRUPT (7 | INTERRUPT)     // 时钟中断
#define MACHINE_EXTERNAL_INTERRUPT (11 | INTERRUPT) // 外部中断
#define ILLEGAL_INSTRUCTION (2 | EXCEPTION)         // 不合法指令异常
#define LOAD_ACCESS_FAULT (5 | EXCEPTION)           // 数据加载异常
#define DISK_PAGE_NUMBER 0x2000                     //32MB硬盘中的交换空间
#define MEM_FAIL 0xffffffff                         //内存分配或者其他操作失败
#define get_pte_v(x) (x & 1)                        //获取pte的v位
#define get_pte_rsw(x) ((x >> 8) & 1)               //获取rsw位
#define SWAPBEGIN 16392                             //交换分区开始的块数
#define SWAPEND 80795                               //交换分区结束的块数
#define MAX_PROCESS_NUM 50                          //最大进程数
#define MAX_GLB_OBJ_NUM 64                          //最大全局对象
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
//预防多重中断嵌套保护的措施
#define PROTECT_IN(can_disable_irq)              \
    do                                           \
    {                                            \
        can_disable_irq = false;                 \
        if ((read_csr(mstatus) & (1 << 3)) != 0) \
        {                                        \
            can_disable_irq = true;              \
            disable_irq();                       \
        }                                        \
    } while (0)
#define PROTECT_OUT(can_disable_irq) \
    do                               \
    {                                \
        if (can_disable_irq == true) \
        {                            \
            enable_irq();            \
        }                            \
    } while (0)