#pragma once
#include "cpupl.h"
#include <exception>
#include <stdlib.h>
extern int server_sockfd;
extern int client_sockfd;
extern unsigned time_counter[2];
extern unsigned csr[];
template <typename... Args>
void flush_to_socket(Args... args)
{
    char buf[1024]{};
    sprintf(buf, args...);
    send(client_sockfd, buf, strlen(buf), 0);
}
union Floating
{
    double dou;
    float flo;
};

class Device
{
public:
    virtual unsigned &operator[](unsigned addr) = 0;
};
class ControlReg
{
public:
    virtual unsigned &operator=(unsigned data) = 0;
    unsigned reg;
};
class SerialReg : public ControlReg
{
public:
    unsigned &operator=(unsigned data)
    {
        reg = data;
        return reg;
    }
};
class Serial : public Device
{
public:
    unsigned &operator[](unsigned addr)
    {
        if (addr == TRANSMEM(0xfffff100)) //控制寄存器
        {
            printf("%c", buf);
            fflush(stdout);
            return creg.reg;
        }
        buf=0;
        return buf; //数据寄存器
    }

private:
    unsigned buf = 0;
    SerialReg creg;
};
class KeyBoard : public Device //可中断传递信息设备
{
public:
    unsigned &operator[](unsigned addr)
    {
        assert(addr == TRANSMEM(0xfffff200));
        now_get = boardbuf;
        return now_get;
    }
    char boardbuf{};

private:
    unsigned now_get{};
};
class Memory : public Device
{
public:
    unsigned &operator[](unsigned addr)
    {
        return this->memory[addr];
    }

private:
    unsigned memory[MEM_MB(64)]{}; // 定义内存,作指令时为unsigned，作数据时由运算决定
};
class Block //硬盘设备
{
public:
    bool setpath(std::string path)
    {
        this->path = path;
        fp = fopen(path.c_str(), "rb+");
        if (fp == nullptr)
        {
            return false;
        }
        has_disk = true;
        return true;
    }
    void close()
    {
        fclose(fp);
    }
    /*读数据操作：rw->addr->ready->data
    写数据操作：rw->addr->data->ready*/
    unsigned &operator[](unsigned addr /*磁盘寄存器地址*/)
    {
        if (addr >= TRANSMEM(0xfffff600) and addr < TRANSMEM(0xfffff800))
        {
            return data[addr - TRANSMEM(0xfffff600)];
        }
        else if (addr == TRANSMEM(0xfffff400))
        {
            return lbaaddr;
        }
        else if (addr == TRANSMEM(0xfffff500))
        {
            if (rand() % 2 == 0)
            {
                ready = 1;
                if (rw == 0) //写数据
                {
                    write_to_file();
                }
                else //读数据
                {
                    read_from_file();
                }
            }
            else
            {
                ready = 0;
            }
            return ready;
        }
        else if (addr == TRANSMEM(0xfffff300))
        {
            return rw;
        }
    }
    FILE *getFp()
    {
        return fp;
    }
    bool has_disk{false}; //是否有disk
private:
    void read_from_file()
    {
        //每512字节为一块
        fseek(fp, lbaaddr << 9, SEEK_SET);
        fread(data, sizeof(unsigned), 128, fp);
    }
    void write_to_file()
    {
        fseek(fp, lbaaddr << 9, SEEK_SET);
        fwrite(data, sizeof(unsigned), 128, fp);
    }
    std::string path;
    FILE *fp;

    unsigned data[128]; //数据寄存器：表示缓冲区数据
    unsigned lbaaddr;   //地址寄存器：表示lba地址
    unsigned ready;     //状态寄存器：最低位有效，表示磁盘已经找到数据
    unsigned rw;        //控制寄存器：最低位有效，1表示读，0表示写
};
class Clock
{
public:
    unsigned &operator[](unsigned addr)
    {
        if (addr == TRANSMEM(0xfffff004))
        {
            return reinterpret_cast<unsigned &>(time_counter[0]); //mtime低地址
        }
        else if (addr = TRANSMEM(0xfffff00c))
        {
            
            return reinterpret_cast<unsigned &>(time_counter[1]); //mtimecmp低地址
        }
    }
};
Serial ttyout;
KeyBoard board;
Memory memo;
Block disk;
Clock clk;
class DeviceManager
{
public:
    DeviceManager() = default;
    unsigned &operator[](ull index)
    {

        if (unsigned(csr[A_SATP] & SATP_MODE(1)) != 0)
        {

            index = mmu(index);
        }
        index = TRANSMEM(index);

        if (index == TRANSMEM(0xfffff000) or index == TRANSMEM(0xfffff100))
        {
            return ttyout[index];
        }
        else if (index == TRANSMEM(0xfffff200))
        {
            return board[index];
        }
        else if (index >= TRANSMEM(0xfffff300) and index < TRANSMEM(0xfffff800))
        {
            return disk[index];
        }
        else if (index == TRANSMEM(0xfffff004) or index == TRANSMEM(0xfffff00c))
        {
            return clk[index];
        }
        else
        {

            return memo[index];
        }
    }
    ull mmu(ull ori_addr)
    {
        //内存管理单元页地址转换与保护，加入访问空异常处理
        ull item1 = GET_PTE1(csr[A_SATP], ori_addr); //一级页表项地址
        unsigned pte1 = memo[TRANSMEM(item1)];       //一级页表项
        if ((pte1 & 1) == 0)                         //V==0?
        {
            FLUSH("pte1 error: 0x%x @ 0x%x\n ", pte1, ori_addr);
            
            throw static_cast<unsigned>(ori_addr);
        }
        memo[TRANSMEM(item1)] |= (1 << 6);     //A位置1
        ull item2 = GET_PTE2(pte1, ori_addr);  //二级页表项地址
        unsigned pte2 = memo[TRANSMEM(item2)]; //二级页表项
        if ((pte2 & 1) == 0)
        {
            FLUSH("pte2 error: 0x%x @ 0x%x\n ", pte2, ori_addr);
            throw static_cast<unsigned>(ori_addr);
        }
        memo[TRANSMEM(item2)] |= (1 << 6);     //A位置1
        ull phy_addr = GET_PA(pte2, ori_addr); //物理地址
        return phy_addr;
    }
};
DeviceManager mem;
