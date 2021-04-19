.text
.globl _interrrupt
_interrrupt:
    //保存上下文到进入对应进程号的ctx中，不改变sp指针
    csrw mscratch, x1//保存x1寄存器
    csrw stval,x3//保存x3寄存器
    lw x1,running//载入running值
    li x3,(36<<2)//载入一个ctx项大小
    mul x1,x1,x3//计算偏移量
    la x3,process//载入process地址
    add x1,x1,x3//计算当前进程ctx的基地址存入x1
    //逐个保存寄存器
    sw x0,0(x1)
    csrr x3,mscratch//载入x1的值
    sw x3,4(x1)//保存x1值
    sw x2,8(x1)//保存x2值
    csrr x3,stval//载入x3的值
    sw x3,12(x1)//保存x3的值
    sw x4,16(x1)//保存x4的值
    sw x5,20(x1)//保存x5的值
    sw x6,24(x1)
    sw x7,28(x1)
    sw x8,32(x1)
    sw x9,36(x1)
    sw x10,40(x1)
    sw x11,44(x1)
    sw x12,48(x1)
    sw x13,52(x1)
    sw x14,56(x1)
    sw x15,60(x1)
    sw x16,64(x1)
    sw x17,68(x1)
    sw x18,72(x1)
    sw x19,76(x1)
    sw x20,80(x1)
    sw x21,84(x1)
    sw x22,88(x1)
    sw x23,92(x1)
    sw x24,96(x1)
    sw x25,100(x1)
    sw x26,104(x1)
    sw x27,108(x1)
    sw x28,112(x1)
    sw x29,116(x1)
    sw x30,120(x1)
    sw x31,124(x1)
    csrr a0,mepc
    csrr a1,mstatus
    csrr a2,mcause
    csrr a3,mtval
    sw a0,128(x1)
    sw a1,132(x1)
    sw a2,136(x1)
    sw a3,140(x1)
    //切换sp到内核0号进程ctx的保存sp中，完成栈切换。后续需要在进程初始化时初始化0号进程的进程局部变量
    la a0,process//载入process表地址
    lw sp,8(a0)//完成系统栈指针切换,必须切换，否则无法处理缺页中断
    //中断服务程序,调入next_ctx
    jal serveinter//进入中断服务程序，cpp中不得更改ctx结构体值，仅仅调入next_ctx
    //切换上下文到next_ctx，不改变sp指针
    la x1,process//载入基地址
    lw x2,running//载入偏移量
    li x3,(36<<2)//载入立即数
    mul x2,x2,x3//载入偏移地址
    add x1,x1,x2//计算最终的进程ctx基地址
    lw x2,8(x1)
    lw x4,16(x1)//载入x4的值
    lw x5,20(x1)//载入x5的值
    lw x6,24(x1)
    lw x7,28(x1)
    lw x8,32(x1)
    lw x9,36(x1)
    lw x10,40(x1)
    lw x11,44(x1)
    lw x12,48(x1)
    lw x13,52(x1)
    lw x14,56(x1)
    lw x15,60(x1)
    lw x16,64(x1)
    lw x17,68(x1)
    lw x18,72(x1)
    lw x19,76(x1)
    lw x20,80(x1)
    lw x21,84(x1)
    lw x22,88(x1)
    lw x23,92(x1)
    lw x24,96(x1)
    lw x25,100(x1)
    lw x26,104(x1)
    lw x27,108(x1)
    lw x28,112(x1)
    lw x29,116(x1)
    lw x30,120(x1)
    lw x31,124(x1)
    lw x3,128(x1)//mepc
    csrw mepc,x3
    lw x3,132(x1)//mstatus
    csrw mstatus,x3
    lw x3,136(x1)//mcause
    csrw mcause,x3
    lw x3,140(x1)//mtval
    csrw mtval,x3
    lw x3,8(x1)
    lw x1,4(x1)
    //中断返回
    mret