#include "table.hpp"
#include "object.hpp"
#include "util.h"
#include "event.hpp"
namespace System
{
    void Keyboard::eventOccur() //键盘中断到来时调用
    {
        systable.mapToProcess([](Process &proc) { proc.recvEvent({EventType::External_Interrupt, keymessage}); });
    }
    bool Swap::writePage(unsigned *data, unsigned disk_ppn)
    {
        return objtable.getObject(deviceid).dis.recvData(data, 8, begin + disk_ppn * 8);
    }
    bool Swap::readPage(unsigned *data, unsigned disk_ppn)
    {
        return objtable.getObject(deviceid).dis.sendData(data, 8, begin + disk_ppn * 8);
    }
    void Keyboard::initialize()
    {
        unsigned addr = 0x4000200; //虚拟地址
        data_port = reinterpret_cast<unsigned *>(addr);
    }

} // namespace System