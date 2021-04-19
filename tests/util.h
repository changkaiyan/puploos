#pragma once
#define NULL 0
using size_t = unsigned;
extern "C"
{
    void *(memset)(void *s, int c, size_t n); //C++编译器自动调用填充类中初始化,不可删除，否则编译不通过
    void __cxa_pure_virtual();                //C++编译器调用纯虚函数时的处理机制
    char *strcpy(char *strDest, const char *strSrc);
}
int createProcess(char *name, void *(*func)(void *), void *arg);
bool exitProcess(int process_id, int retval);
bool sleepProcess(int tick);
bool resumeProcess(int process_id);

void *kernelMalloc(size_t memsize);
bool exitProcess(int retval);
bool kernelFree(void *addr);