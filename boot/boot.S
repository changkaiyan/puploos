.text
.globl _start
.align 4
_start:
    //bootloader：关中断、建临时栈、读入6M os到0x100000地址
    csrci mstatus, (1<<3)  //关中断
    li sp,0x100000 // 1M位置临时建栈
    jal bootloader  // 跳转入C语言编写的bootloader
    ebreak //查看处理机状态

    
