#pragma once
#include "asm.h"
namespace System
{
    using ppa = unsigned;
    using size_t = unsigned;
    class Memory
    {
    public: //初始化与虚拟内存管理（放到启动器中）
        Memory &initializePage();
        Memory &initializeMemory();
        /*初始化重映射设备的虚拟存储器映射*/
        Memory &initializeDeviceMemory();
        /*table.hpp中的存储器管理是虚拟存储器管理，memory.hpp是物理存储器管理*/
        /*addr_to_memsize记录分配的空间大小，函数返回虚拟地址*/
        /*为了表示方便，PUPLOOS的虚拟地址和物理地址默认同等映射*/
        void *kernelMalloc(size_t memsize);

        /*输入虚拟地址释放*/
        bool kernelFree(void *addr);
        /**
         * @brief 按照磁盘中的地址换入特定的页面，用于请求调页
         * 
         * @param disk_ppn 磁盘页号
         */
        void swap_in_spec(unsigned disk_ppn);
        int _test_getphyfreenumber();
        //用于中断判断访存异常和缺页中断
        bool page_is_exist(unsigned virtual_addr)
        {
            unsigned *page_table2_p = reinterpret_cast<unsigned *>(page_table2); //二维列表一维化
            return get_pte_rsw(page_table2_p[get_ppn(virtual_addr)])==1;
        }

    private: //物理内存管理
             /**
     * @brief 查找需要换出的页面
     * 
     * @return unsigned 物理页面ppn号
     */
        unsigned find_page_to_swapout();

        /*空闲的物理页面数，设计时考虑全局表中分配内存合法性*/
        int getFreeNumber();
        //返回物理页号
        ppa pageMalloc();
        /*输入物理页号*/
        Memory &pageFree(ppa mem_ppn);
        /**
         * @brief 释放虚拟页面
         * 
         * @param disk_ppn 
         * @return Memory& 存储器对象
         */
        Memory &diskFree(ppa disk_ppn);
        /*最大连续虚拟页面项,输入数目，返回虚拟页号virtual_mem_ppn*/
        size_t find_continue_page(int numpage);
        /*swap区域仅处理物理存储&磁盘存储区*/
        /*查找磁盘中的空白页面并换入，用于malloc*/
        void swap_in_blank();

        /**
         * @brief 交换磁盘和物理内存中对应ppn号的页面
         * 
         * @param disk_ppn 磁盘中页框ppn
         * @param mem_ppn 物理内存中ppn
         */
        void _swap(unsigned disk_ppn, unsigned mem_ppn);
        unsigned _mem2virtualppn(unsigned mem_ppn);
        void _swap_page(unsigned disk_ppn, unsigned virtual_mem_ppn);
        //内核进程占四个一级页表项, 16MB
        __attribute__((aligned(PAGESIZE))) unsigned page_table[PAGE_ITEM_NUMBER]{0};                    //一级页表（静态分配）
        __attribute__((aligned(PAGESIZE))) unsigned page_table2[PAGE_ITEM_NUMBER][PAGE_ITEM_NUMBER]{0}; //二级页表（静态分配）
        unsigned memory_map[PAGENUM / 32]{0};                                                           //内存使用统计64MB/4KB/32bit(位图)
        unsigned disk_map[DISK_PAGE_NUMBER / 32]{0};                                                    //硬盘交换空间使用统计32MB/4KB/32bit(位图)
        size_t ppn_to_memppn[VIRTUAL_PAGENUM]{0};                                                       //记录分配与释放ppn与内存块大小的关系
        int freenum{0};                                                                                 //空闲的页面数
        unsigned page_tmp[1024]{0};                                                                      //页面对换区
        int access_freq[PAGENUM + DISK_PAGE_NUMBER]{0};                                                 //统计可对换存储器空间中页面换入次数
    };
} // namespace System