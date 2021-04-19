#pragma once
//信号量机制
namespace System
{
    class Semaphore
    {
    public:
        void initialize(int n);
        void P();
        void V();

    private:
        int value;
        int wait_process[50];
    };
} // namespace System