#include "print.hpp"
#include "table.hpp"
#include "asm.h"
static void putc(int objid, char c)
{

    System::objtable.getObject(objid).ttyS.recvData(&c, 1);
}

static void printint(int objid, int xx, int base, int sgn)
{

    static char digits[] = "0123456789ABCDEF";
    char buf[16]{0};
    int i, neg;
    unsigned x;
    neg = 0;
    if (sgn && xx < 0)
    {
        neg = 1;
        x = -xx;
    }
    else
    {
        x = xx;
    }

    i = 0;
    do
    {
        buf[i++] = digits[x % base];

    } while ((x /= base) != 0);
    if (neg)
        buf[i++] = '-';

    while (--i >= 0)
        putc(objid, buf[i]);
}
/**
 * @brief 
 * 
 * @param objid 对象编号
 * @param fmt 格式字符串
 * @param ... 最多传入6个参数
 */
void System::printf(int objid, const char *fmt, ...)
{
    bool can{false};
    PROTECT_IN(can);
    unsigned args[8];
//可变参数保存
#define EXTARG(num) asm volatile("sw a" #num ",%0" \
                                 : "=m"(args[num]))
    EXTARG(0);
    EXTARG(1);
    EXTARG(2);
    EXTARG(3);
    EXTARG(4);
    EXTARG(5);
    EXTARG(6);
    EXTARG(7);
    char *s;
    int c, i, state;
    unsigned *ap;
    state = 0;
    //函数调用取得可变参数的方法需要变化
    ap = args + 2;
    for (i = 0; fmt[i]; i++)
    {
        c = fmt[i] & 0xff;
        if (state == 0)
        {
            if (c == '%')
            {
                state = '%';
            }
            else
            {
                putc(objid, c);
            }
        }
        else if (state == '%')
        {
            if (c == 'd')
            {
                printint(objid, *ap, 10, 1);
                ap++;
            }
            else if (c == 'x' || c == 'p')
            {
                printint(objid, *ap, 16, 0);
                ap++;
            }
            else if (c == 's')
            {
                s = (char *)*ap;
                ap++;
                if (s == 0)
                    s = "(null)";
                while (*s != 0)
                {
                    putc(objid, *s);
                    s++;
                }
            }
            else if (c == 'c')
            {
                putc(objid, *ap);
                ap++;
            }
            else if (c == '%')
            {
                putc(objid, c);
            }
            else
            {
                putc(objid, '%');
                putc(objid, c);
            }
            state = 0;
        }
    }
    PROTECT_OUT(can);
}
