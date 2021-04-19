#include "memory.hpp"
#include "asm.h"
#include "table.hpp"
#include "util.h"
#include "print.hpp"
namespace System
{
    void Memory::_swap(unsigned disk_ppn, unsigned mem_ppn)
    {

        unsigned *page_table2_p = reinterpret_cast<unsigned *>(page_table2); //二维列表一维化

        unsigned virtual_mem_ppn = _mem2virtualppn(mem_ppn); //查找物理内存对应的虚拟内存ppn
        //更新并交换页面
        _swap_page(disk_ppn, virtual_mem_ppn);

        //更新页表项
        for (int i{0}; i < 1024 * 1024; ++i)
        {
            //查找并更新disk_ppn所在的页表项
            if (get_ppn_from_pte(page_table2_p[i]) == disk_ppn and get_pte_rsw(page_table2_p[i]) == 1 and get_pte_v(page_table2_p[i]) == 0)
            {
                page_table2_p[i] = page_item(mem_ppn, 1, 0, 0, 0, 1);
            }
            else if (get_ppn_from_pte(page_table2_p[i]) == mem_ppn and get_pte_rsw(page_table2_p[i]) == 1 and get_pte_v(page_table2_p[i]) == 1) //查找并更新mem_ppn所在的页表项
            {
                page_table2_p[i] = page_item(disk_ppn, 0, 0, 0, 0, 1);
            }
        }

        //对换access统计区
        auto tmp = access_freq[mem_ppn];
        access_freq[mem_ppn] = access_freq[disk_ppn + PAGENUM];
        access_freq[disk_ppn + PAGENUM] = tmp;
        access_freq[mem_ppn]++; //增加换入页面的权重
    }
    unsigned Memory::_mem2virtualppn(unsigned mem_ppn)
    {
        unsigned *page_table2_p = reinterpret_cast<unsigned *>(page_table2); //二维列表一维化
        for (int i{0}; i < 1024 * 1024; ++i)
        {
            if (get_ppn_from_pte(page_table2_p[i]) == mem_ppn and get_pte_rsw(page_table2_p[i]) == 1 and get_pte_v(page_table2_p[i]) == 1) //查找mem_ppn所在的页表项
            {
                return i;
            }
        }
        return MEM_FAIL;
    }
    void Memory::_swap_page(unsigned disk_ppn, unsigned virtual_mem_ppn)
    {
        //交换disk_ppn号和virtual_mem_ppn号的页面数据
        unsigned *mm_p = reinterpret_cast<unsigned *>(virtual_mem_ppn << 12);
        for (int i{0}; i < 1024; ++i) //将存储器页面交换到临时对换区
        {
            page_tmp[i] = mm_p[i];
        }
        //将磁盘中页面交换到mm_p指针处
        objtable.getObject(3).swa.readPage(mm_p, disk_ppn);
        //将临时对换区数据交换到磁盘中
        objtable.getObject(3).swa.writePage(page_tmp, disk_ppn);
    }
    Memory &Memory::diskFree(ppa disk_ppn)
    {
        int index0 = disk_ppn >> 5;
        int index1 = disk_ppn % 32;
        if ((disk_map[index0] & (1 << index1)) == 0) //页面已经释放过，不改变freenum数
        {
            return *this;
        }
        disk_map[index0] &= ~(1 << index1);
        access_freq[disk_ppn + PAGENUM] = 0; //清零对换次数统计
        freenum++;
        return *this;
    }
    unsigned Memory::find_page_to_swapout()
    {
        //在非内核区中查找中断次数最少的存储器中第一个页面
        unsigned min_index{0};                    //最小缺页次数的ppn
        unsigned min_access{0xffffffff};          //最小缺页次数
        for (int i{0}; i < KERNEL_BEGIN_PPN; ++i) //内核区不允许对换
        {
            if (access_freq[i] < min_access)
            {
                min_index = i;
                min_access = access_freq[i];
            }
        }
        //返回此页面的ppn号
        return min_index;
    }
    int Memory::getFreeNumber()
    {
        return freenum;
    }
    Memory &Memory::initializeDeviceMemory()
    {
        unsigned *page_table2_p = reinterpret_cast<unsigned *>(page_table2); //二维列表一维化
        page_table2_p[0x4000] = page_item(0xfffff, 1, 0, 0, 0, 1);           //把以0xfffffppn为起点的设备映射一个ppn->虚存0x4000ppn上
        return *this;
    }
    void Memory::swap_in_spec(unsigned disk_ppn)
    {
        //对换memory_map和disk_map相应位
        unsigned mem_ppn = find_page_to_swapout();
        unsigned tmp;
        int q{mem_ppn >> 5}, p{mem_ppn % 32};
        tmp = (memory_map[q] & (1 << p)); //保存内存中页面标志位
        memory_map[q] |= (1 << p);
        q = disk_ppn >> 5;
        p = disk_ppn % 32;
        if (tmp == 0) //内存中页面是空页
        {
            disk_map[q] &= ~(1 << p);
        }
        else //内存中页面非空
        {
            disk_map[q] |= (1 << p);
        }
        _swap(disk_ppn, mem_ppn);
    }
    Memory &Memory::pageFree(ppa mem_ppn)
    {
        int index0 = mem_ppn >> 5;
        int index1 = mem_ppn % 32;
        if ((memory_map[index0] & (1 << index1)) == 0) //页面已经释放过，不改变freenum数
        {
            return *this;
        }
        memory_map[index0] &= ~(1 << index1);
        access_freq[mem_ppn] = 0; //清零对换次数统计
        freenum++;
        return *this;
    }
    void Memory::swap_in_blank()
    {
        unsigned disk_ppn, mem_ppn;
        //找空白页面，更新disk_map
        for (int i{0}; i < DISK_PAGE_NUMBER / 32; ++i)
        {
            if (disk_map[i] != 0xffffffff) //页面有空余
            {
                for (int j{0}; j < 32; ++j)
                {
                    if ((disk_map[i] & (1 << j)) == 0) //找到空余页面
                    {
                        disk_map[i] |= (1 << j); //换入已经使用的页面
                        disk_ppn = i * 32 + j;   //从0开始的ppa
                        goto endloop;            //跳出循环
                    }
                }
            }
        }
    endloop:
        //计算物理mem_ppn
        mem_ppn = find_page_to_swapout();
        //更新memory_map，删去此页面
        int q{mem_ppn >> 5}, p{mem_ppn % 32};
        memory_map[q] &= ~(1 << p);
        _swap(disk_ppn, mem_ppn); //交换页面并更新页表指向
    }
    size_t Memory::find_continue_page(int numpage)
    {
        unsigned *page_table2_p = reinterpret_cast<unsigned *>(page_table2); //二维列表一维化
        int virtual_mem_ppn_base{0}, virtual_mem_ppn_end{0};
        while (virtual_mem_ppn_base < PAGE_ITEM_NUMBER * PAGE_ITEM_NUMBER - numpage) //动态规划法查找定长连续子序列
        {
            unsigned not_valid = 0;
            for (virtual_mem_ppn_end = virtual_mem_ppn_base; virtual_mem_ppn_end < virtual_mem_ppn_base + numpage; ++virtual_mem_ppn_end)
            {
                if (get_pte_rsw(page_table2_p[virtual_mem_ppn_end]) != 0) //有页表项被占用
                {
                    not_valid = 1;
                    break;
                }
            }
            if (not_valid == 0) //[i,j)页表项均没有被占用
            {
                return virtual_mem_ppn_base; //返回页表项下标virtual_mem_ppn
            }
            virtual_mem_ppn_base = virtual_mem_ppn_end + 1;
        }
        return MEM_FAIL;
    }
    Memory &Memory::initializePage()
    {
        write_csr(satp, get_ppn(page_table)); //初始化页表寄存器satp

        for (int i{0}; i < PAGE_ITEM_NUMBER; ++i) //初始化一级页表中所有项，因为是静态分配
        {
            page_table[i] = page_item((get_ppn(page_table2[i])), 1, 0, 0, 0, 1);
        }
        for (int i{0}; i < PAGE_ITEM_NUMBER; ++i)
        {

            //初始化内核二级页表
            for (int j{0}; j < PAGE_ITEM_NUMBER; j++)
            {
                if (i >= 12 and i < 16)
                    //在48MB->0x3000PPN基础上每4MB增加0x400(1024)PPN
                    page_table2[i][j] = page_item(j + KERNEL_BEGIN_PPN + (PAGE_ITEM_NUMBER * (i - 12)), 1, 0, 0, 0, 1);
                else
                    page_table2[i][j] = 0;
            }
        }

        //开启分页
        unsigned tmppage;
        tmppage = read_csr(satp);
        tmppage |= PAGE_MODE;
        write_csr(satp, tmppage);
        return *this;
    }
    int Memory::_test_getphyfreenumber()
    {
        int number{0};
        for (int i{0}; i < PAGENUM / 32; ++i)
        {
            if (memory_map[i] != 0xffffffff) //页面有空余
            {
                for (int j{0}; j < 32; ++j)
                {
                    if ((memory_map[i] & (1 << j)) == 0) //找到空余页面
                    {
                        number++;
                    }
                }
            }
        }
        return number;
    }
    void *Memory::kernelMalloc(size_t memsize)
    {

        if (memsize == 0)
        {
            return reinterpret_cast<void *>(0xffffffff);
        }
        bool can{false};
        PROTECT_IN(can);
        /*第一级页表始终不清空，不处理，不分配；内存的管理仅针对第二级页表。*/
        int numberppn = (memsize + PAGESIZE - 1) / PAGESIZE; //总共需要的ppn数目(向上取整)
        if (numberppn > getFreeNumber())                     //请求的内存容量大于内存空闲的容量
        {
            PROTECT_OUT(can);
            return reinterpret_cast<void *>(0xffffffff);
        }                                                                    //更新空闲内存块数
        size_t pageindex;                                                    //物理块(虚页)号
        unsigned *page_table2_p = reinterpret_cast<unsigned *>(page_table2); //二维列表一维化
        size_t virtual_page_index = find_continue_page(numberppn);           //找到连续的虚拟页面ppn
        if (virtual_page_index == MEM_FAIL)                                  //虚拟空间不够
        {
            PROTECT_OUT(can);
            return reinterpret_cast<void *>(MEM_FAIL);
        }
        //逐个分配页面
        for (int i{0}; i < numberppn; ++i)
        {
            pageindex = pageMalloc(); //分配物理页面
            if (pageindex == MEM_FAIL)
            {
                swap_in_blank();          //换入页面。这里不单独列diskmalloc是根据进程局部性原理，malloc后就要使用
                pageindex = pageMalloc(); //因为提前计算好了，所以直接分配
            }

            page_table2_p[virtual_page_index + i] = page_item(pageindex, 1, 0, 0, 0, 1); //分配虚拟内存号
        }
        ppn_to_memppn[virtual_page_index] = numberppn; //记录虚拟地址对应的内存大小

        PROTECT_OUT(can);
        return reinterpret_cast<void *>(virtual_page_index << 12);
    }
    Memory &Memory::initializeMemory()
    {
        //物理内存初始化
        memset(memory_map, 0, 4 * PAGENUM / 32);
        memset(disk_map, 0, 4 * DISK_PAGE_NUMBER / 32);
        memset(ppn_to_memppn, 0, 4 * VIRTUAL_PAGENUM);
        memset(page_tmp, 0, 4 * 128);
        memset(access_freq, 0, 4 * (PAGENUM + DISK_PAGE_NUMBER));

        for (int i{KERNEL_BEGIN_PPN / 32}; i < (PAGENUM / 32); ++i)
        {
            memory_map[i] = 0xffffffff;
        }

        freenum = KERNEL_BEGIN_PPN + DISK_PAGE_NUMBER; //内存中的虚拟页面数+硬盘中的虚拟页面数
        return *this;
    }
    bool Memory::kernelFree(void *addr)
    {
        bool can{false};
        PROTECT_IN(can);
        size_t baseppn = get_ppn(addr); //虚拟基页号

        size_t freepage = ppn_to_memppn[baseppn]; //释放的内存页ppn数
        if (freepage == 0)                        //查找不到地址对应的分配大小
        {
            PROTECT_OUT(can);
            return false;
        }
        ppn_to_memppn[baseppn] = 0;
        /*第一级页表始终不清空，不处理，不分配；内存的管理仅针对第二级页表。*/
        unsigned *page_table2_p = reinterpret_cast<unsigned *>(page_table2); //二维列表一维化
        for (int i{0}; i < freepage; ++i)
        {
            if (get_pte_v(page_table2_p[baseppn + i]) == 0) //页在磁盘上
            {
                diskFree(get_ppn_from_pte(page_table2_p[baseppn + i])); //释放磁盘中页面
            }
            else //页在内存中
            {
                pageFree(get_ppn_from_pte(page_table2_p[baseppn + i])); //释放物理页面
            }
            page_table2_p[baseppn + i] = 0; //释放第二级页表
        }
        PROTECT_OUT(can);
        return true;
    }
    ppa Memory::pageMalloc()
    {
        for (int i{0}; i < PAGENUM / 32; ++i)
        {
            if (memory_map[i] != 0xffffffff) //页面有空余
            {
                for (int j{0}; j < 32; ++j)
                {
                    if ((memory_map[i] & (1 << j)) == 0) //找到空余页面
                    {
                        memory_map[i] |= (1 << j);
                        freenum--;
                        return i * 32 + j; //从0开始的ppa
                    }
                }
            }
        }

        return MEM_FAIL; //没有找到
    }
} // namespace System