#pragma once
#include "semaphore.hpp"
namespace System
{
    /*抽象对象，控制继承对象的公有接口，但是不可以多态访问*/
    class VirtualObject
    {
    public:
        bool recvData(void *arg, int number) {} //接受数据
        bool sendData(void *arg, int number) {} //向外发送数据
        bool setControl(int control) {}         //转换控制状态
        bool getStatus(unsigned &status) {}     //转换通用状态
    };
    class Serial : public VirtualObject
    {
    public:
        void initialize() //对象构造方法必须以initialize方式写出
        {
            data_port = reinterpret_cast<unsigned *>(0x4000000);
            flush_port = reinterpret_cast<unsigned *>(0x4000100);
        }
        bool recvData(void *arg, int number) //接受数据
        {
            char *data = reinterpret_cast<char *>(arg);
            unsigned dat = *data;
            *data_port = dat;
            *flush_port = 1;
            return true;
        }
        bool sendData(void *arg, int number) //向外发送数据
        {
            return false;
        }
        bool setControl(int control) //转换控制状态
        {
            return false;
        }
        bool getStatus(unsigned &status)
        {
            return false;
        }

    private:
        unsigned *data_port;
        unsigned *flush_port;
    };
    class Keyboard : public VirtualObject
    {
    public:
        void initialize();
        bool recvData(void *arg, int number) //接受数据
        {
            return false;
        }
        bool sendData(void *arg, int number) //向外发送数据
        {
            unsigned *tmp = reinterpret_cast<unsigned *>(arg);
            *tmp = *data_port;
            return true;
        }
        bool setControl(int control) //转换控制状态
        {
            return false;
        }
        bool getStatus(unsigned &status) //转换通用状态
        {
            return false;
        }
        void eventOccur(); //键盘中断到来时调用

    private:
        unsigned *data_port;
    };
    class Disk : public VirtualObject
    {
    public:
        void initialize()
        {
            data_port = reinterpret_cast<unsigned *>(0x4000600);  //数据端口为缓冲区，共512字节
            addr_port = reinterpret_cast<unsigned *>(0x4000400);  //lba地址区
            rw_port = reinterpret_cast<unsigned *>(0x4000300);    //读写使能端口
            ready_port = reinterpret_cast<unsigned *>(0x4000500); //数据准备好端口
        }
        bool recvData(void *arg, int number, int lbaaddr) //接收数据，number为数据块数，按512字节计算
        {
            bool can{false};
            PROTECT_IN(can);
            unsigned *p = reinterpret_cast<unsigned *>(arg);
            for (int i{0}; i < number; ++i)
            {
                write_sec_data(reinterpret_cast<unsigned *>(p + 128 * i), lbaaddr + i);
            }
            PROTECT_OUT(can);
            return true;
        }
        bool sendData(void *arg, int number, int lbaaddr) //从磁盘中读出数据，number为数据块数，按512字节计算
        {
            bool can{false};
            PROTECT_IN(can);
            unsigned *p = reinterpret_cast<unsigned *>(arg);
            for (int i{0}; i < number; ++i)
            {
                read_sec_data(reinterpret_cast<unsigned *>(p + 128 * i), lbaaddr + i);
            }
            PROTECT_OUT(can);
            return true;
        }
        bool setControl(int control) //转换控制状态
        {
            return false;
        }
        bool getStatus(unsigned &status) //转换通用状态
        {
            return false;
        }

    private:
        /*从磁盘中读出512字节数据到addr中*/
        void read_sec_data(unsigned *addr, unsigned lbraddr)
        {
            *rw_port = 1;
            *addr_port = lbraddr;
            while (*ready_port == 0)
                ;
            for (int i = 0; i < 128; ++i)
            {
                *(addr + i) = *(data_port + i);
            }
        }
        /*向磁盘写入512字节数据到addr中*/
        void write_sec_data(unsigned *addr, unsigned lbraddr)
        {
            *rw_port = 0;
            *addr_port = lbraddr;
            for (int i = 0; i < 128; ++i)
            {
                *(data_port + i) = *(addr + i);
            }
            while (*ready_port == 0)
                ;
        }
        unsigned *data_port;
        unsigned *addr_port;
        unsigned *rw_port;
        unsigned *ready_port;
    };
    /*交换存储区：[4kb+8MB，4kb+8MB+32MB)*/
    class Swap //分区对象
    {
    public:
        void initialize(int beg, int en, int device)
        {
            begin = beg;
            size = en;
            deviceid = device;
        }
        /**
         * @brief 向磁盘交换区写入数据
         * 
         * @param data 数据区基地址
         * @param number 数据区元素数量,4kb
         * @param disk_ppn 磁盘按ppn计算的4kb页面号
         * @return true 写入成功
         * @return false 写入失败
         */
        bool writePage(unsigned *data, unsigned disk_ppn);
        bool readPage(unsigned *data, unsigned disk_ppn);

    private:
        int begin;
        int size;
        int deviceid;
    };
    enum ClockReg
    {
        mtime,
        mtimecmp
    };
    class Clock //时钟设备
    {
    public:
        void initialize()
        {
            mtime_addr = reinterpret_cast<unsigned *>(0x4000004);
            mtimecmp_addr = reinterpret_cast<unsigned *>(0x400000c);
        }
        bool recvData(ClockReg reg, unsigned data)//改变mtime或者mtimecmp寄存器的值
        {
            if (reg == ClockReg::mtime)
            {
                *mtime_addr = data;
            }
            else
            {
                *mtimecmp_addr = data;
            }
            return true;
        }
        bool sendData(ClockReg reg, unsigned &data)
        {
            if (reg == ClockReg::mtime)
            {
                data = *mtime_addr;
            }
            else
            {
                data = *mtimecmp_addr;
            }
            return true;
        }

    private:
        unsigned *mtime_addr, *mtimecmp_addr;
    };

    REGISTER_OBJECT(Serial ttyS; Keyboard key; Disk dis; Swap swa; Clock clk;Semaphore sema;); //不同对象间用分号隔开

} // namespace System