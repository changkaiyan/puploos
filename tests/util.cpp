#include "util.h"
#include "asm.h"
#include "cpu.h"

void *memset(void *s, int c, size_t n)
{
    if (NULL == s || n < 0)
        return NULL;
    char *tmpS = (char *)s;
    while (n-- > 0)
        *tmpS++ = c;
    return s;
}
void __cxa_pure_virtual()
{
    ebreak();
    return;
}
char *strcpy(char *strDest, const char *strSrc)
{
    char *p = NULL;
    if (strDest == NULL || strSrc == NULL)
    {
        return NULL;
    }
    p = strDest;
    while ((*strDest++ = *strSrc++) != '\0')
        ;
    *strDest = '\0';
    return p;
}

int createProcess(char *name, void *(*func)(void *), void *arg)
{
    ecall(1, name, func, arg);
    return retvalue[running].unsign;
}

bool exitProcess(int retval)
{
    ecall(2, retval,0, 0);
    return retvalue[running].unsign;
}
bool sleepProcess(int tick)
{
    ecall(3, tick, 0, 0);
    return retvalue[running].unsign;
}
bool resumeProcess(int process_id)
{
    ecall(4,process_id,0,0);
    return retvalue[running].unsign;
}

void *kernelMalloc(size_t memsize)
{
    ecall(5,memsize,0,0);
    return retvalue[running].void_p;

}

bool kernelFree(void *addr)
{
    ecall(6,addr,0,0);
    return retvalue[running].unsign;
}