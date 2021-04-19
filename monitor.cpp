#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
int sockfd = -1;
void sighandle(int sig)
{
    close(sockfd);
    printf("\n");
    exit(0);
}
int main()
{
    signal(SIGSEGV, sighandle);
    signal(SIGINT, sighandle);
    signal(SIGHUP, sighandle);
    signal(SIGKILL, sighandle);
    signal(SIGTSTP, sighandle);
    signal(SIGQUIT, sighandle);

    int len = 0;
    struct sockaddr_in address;
    int result;
    char buf = {};
    //创建流套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd != -1);
    //设置要连接的服务器的信息
    address.sin_family = AF_INET;                     //使用网络套接字
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); //服务器地址
    address.sin_port = htons(8001);                   //服务器所监听的端口
    len = sizeof(address);
    //连接到服务器
    result = connect(sockfd, (struct sockaddr *)&address, len);

    if (result == -1)
    {
        perror("不能连接到虚拟机！\n");
        exit(1);
    }
    printf("虚拟机连接成功！\n");
    while (true)
    {
        buf = 0;

        if (recv(sockfd, &buf, sizeof(buf), 0) == -1)
        {
            break;
        }
        if (buf == -1)
            break;
        printf("%c", buf);
    }
    close(sockfd);
    exit(0);
}