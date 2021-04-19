#pragma once
//进程间事件通信机制
enum class EventType
{
    External_Interrupt, //外部中断
    Inter_Message,      //进程间通信事件
    Segment_Fault       //内存访问错误（段错误）
};
const inline unsigned keymessage = 0; //键盘消息

struct Event
{
    EventType event;
    unsigned message;
};