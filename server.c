#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>

#include "thrmgr.h"
#include "server.h"

void event_write()
{

}

void event_read(void *arg)
{
    int rlen = 0;
    char rbuf[RW_BUFFER_LENGTH] = {0};
    rwdata_t *data = (rwdata_t *)arg;
    int socketfd = data->fd;

    if (socketfd < 0)
    {
        return;
    }
    if ( (rlen = read(socketfd, rbuf, RW_BUFFER_LENGTH)) < 0 )
    {
        if (errno == ECONNRESET) {
            close(socketfd);
            return;
        }
    } 
    else if (rlen == 0)
    {
        LOG_INFO("client close connect, close fd\n");
        close(socketfd);
        return;
    }

    LOG_INFO("read data : %s\n", rbuf);

    sleep(1);

    /* FOR TEST */
    char wbuf[5] = "over.";
    write(socketfd, wbuf, 5);
    

    /*
    //设置用于写操作的文件描述符
    ev.data.fd=sockfd;
    //设置用于注册的写操作事件
    ev.events=EPOLLOUT|EPOLLET;
    //修改sockfd上要处理的事件为EPOLLOUT
    epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
    */
}

int setnonblocking(int sock)
{
    int opts;
    opts = fcntl(sock,F_GETFL);
    if(opts < 0)
    {
        LOG_ERR("fcntl(sock,F_GETFL) failed.\n")
        return FAILURE;
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts) < 0)
    {
        LOG_ERR("fcntl(sock,F_SETFL,opts) failed.\n")
        return FAILURE;

    }
}

int tcp_server(char *ip, int port)
{
    int ret = FAILURE;
    int i, maxi, listenfd, connfd, sockfd, epfd, nfds;
    ssize_t n;
    socklen_t clilen;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;

    //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件
    struct epoll_event ev, events[20];
    //生成用于处理accept的epoll专用的文件描述符

    threadpool_t *threadpool = thrmgr_new(USE_MAX_THREAD_NUM, USE_THREAD_TIMEOUT, event_read);

    epfd = epoll_create(256);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    //把socket设置为非阻塞方式
    setnonblocking(listenfd);

    //设置与要处理的事件相关的文件描述符
    ev.data.fd = listenfd;
    //设置要处理的事件类型

    ev.events = EPOLLIN|EPOLLET;
    //ev.events=EPOLLIN;

    //注册epoll事件
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd,&ev);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(atoi(ip));

    serveraddr.sin_port = htons(port);
    ret = bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (FAILURE == ret)
    {
        LOG_ERR("bind failed, errno:%d\n", errno);
        close(listenfd);
        return FAILURE;
    }
    
    ret = listen(listenfd, LISTENQ);
    if (FAILURE == ret)
    {
        LOG_ERR("listen failed, errno:%d\n", errno);
        close(listenfd);
        return FAILURE;
    }

    
    maxi = 0;
    for ( ; ; ) {
        //等待epoll事件的发生

        nfds=epoll_wait(epfd,events,20,500);
        //处理所发生的所有事件

        for(i=0;i<nfds;++i)
        {
            if(events[i].data.fd==listenfd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
            {
                LOG_INFO("NEW CLIENT\n")
                connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
                if(connfd == -1){
                    LOG_ERR("accept fd < 0, errno:%d\n", errno);
                    continue;    
                }
                //setnonblocking(connfd);

                char *str = inet_ntoa(clientaddr.sin_addr);
                //设置用于读操作的文件描述符

                ev.data.fd=connfd;
                //设置用于注测的读操作事件

                ev.events=EPOLLIN|EPOLLET;
                //ev.events=EPOLLIN;

                //注册ev
                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
            }
            else if(events[i].events&EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
            {
                LOG_INFO("READ EVENT\n")
                rwdata_t *data = (rwdata_t *)malloc(sizeof(rwdata_t));
                data->fd = events[i].data.fd;
                ret = thrmgr_dispatch(threadpool, (void*)data);
                if (SUCCESS != ret)
                {
                    LOG_ERR("thrmgr_dispatch return failure.");
                }

            }
            else if(events[i].events&EPOLLOUT) // 如果有数据发送
            {
                LOG_INFO("WRITE EVENT\n")
            /*
                sockfd = events[i].data.fd;
                write(sockfd, line, n);
                //设置用于读操作的文件描述符

                ev.data.fd=sockfd;
                //设置用于注测的读操作事件

                ev.events=EPOLLIN|EPOLLET;
                //修改sockfd上要处理的事件为EPOLIN

                epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
                */
            }
        }
    }
}
