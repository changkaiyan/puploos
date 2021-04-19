//处理器上下文管理
#include "cpu.h"

Context __attribute__((packed)) process[MAX_PROCESS_NUM]{}; //处理器进程上下文组
int running;                                                //当前运行进程下标（进程号）
bool sleep_running[MAX_PROCESS_NUM]{false};                                  //当前进程需要休眠
bool exit_running[MAX_PROCESS_NUM]{false};//当前进程需要回收
Run retvalue[MAX_PROCESS_NUM];                              //系统调用返回值