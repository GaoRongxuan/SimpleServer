#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>

int main()
{
    int ret = 0;
    int listenfd;
    int clientfd;
    struct sockaddr_in serveraddr;
    
    struct sockaddr_in clientaddr;
    socklen_t clilen;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //把socket设置为非阻塞方式

    //setnonblocking(listenfd);

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi("127.0.0.1"));

    serveraddr.sin_port = htons(8890);
    ret = bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (-1 == ret)
    {
        printf("bind failed, errno:%d\n", errno);
        close(listenfd);
        return -1;
    }
    
    ret = listen(listenfd, 5);
    if (-1 == ret)
    {
        printf("listen failed, errno:%d\n", errno);
        close(listenfd);
        return -1;
    }

    clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
    if(clientfd == -1){
        printf("accept fd < 0, errno:%d\n", errno);
        close(listenfd);
        return -1;
    }

    char rbuf[10] = {0};
    char wbuf[6] = "return";

    read(clientfd, rbuf, 9);
    if (-1 == ret)
    {
        printf("read failed, errno:%d\n", errno);
        close(clientfd);
        close(listenfd);
        return -1;
    }
    printf("read %s\n", rbuf);

    ret = write(clientfd, wbuf, 6);
    if (-1 == ret)
    {
        printf("write failed, errno:%d\n", errno);
        close(clientfd);
        close(listenfd);
        return -1;
    }

    close(clientfd);
    close(listenfd);

    return 0;
}
