#include "semaphore.hpp"
#include "table.hpp"
#include "cpu.h"
#include "util.h"
namespace System
{
    void Semaphore::P()
    {
        bool can{false};
        PROTECT_IN(can);
        value--;
        if (value >= 0)
        {
            PROTECT_OUT(can);
            return;
        }
        wait_process[running] = 1;
        PROTECT_OUT(can);
        systable.sleepProcess(-1);
    }
    void Semaphore::initialize(int n)
    {
        value = n;
        memset(wait_process, 0, 50*sizeof(int));
    }
    void Semaphore::V()
    {
        bool can{false};
        PROTECT_IN(can);
        value++;
        if (value > 0)
        {
            PROTECT_OUT(can);
            return;
        }
        for (int i{}; i < 50; ++i)
        {
            if (wait_process[i] == 1)
                systable.resumeProcess(i);
        }
        PROTECT_OUT(can);
    }

} // namespace System