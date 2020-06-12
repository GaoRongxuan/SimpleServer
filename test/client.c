#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>

#define DST_IP "127.0.0.1"
#define DST_PORT 8890

int dosock()
{
    int socketfd = 0;
    int ret;
    struct sockaddr_in servaddr;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(DST_PORT);
    inet_pton(AF_INET,DST_IP,&servaddr.sin_addr);

    ret = connect(socketfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr));
    if (0 != ret)
    {
        printf("connect failed, err:%d\n", errno);
        close(socketfd);
        return -1;
    }

    char rbuf[5] = "hello";
    char wbuf[10] = {0};

    ret = write(socketfd, rbuf, 5);
    if (-1 == ret)
    {
        printf("write failed, errno:%d\n", errno);
        close(socketfd);
        return -1;
    }

    read(socketfd, wbuf, 9);
    if (-1 == ret)
    {
        printf("read failed, errno:%d\n", errno);
        close(socketfd);
        return -1;
    }
    //printf("read %s\n", wbuf);

    close(socketfd);
}

void *thread_worker(void*arg)
{
    int i = 0;

    for (i = 0; i < 2000; i++)
    {
        dosock();
    }
}

int main(int argc, char *argv[])
{
    pthread_t thr_id;

    int i = 0;
    for (i = 0; i < 30; i++)
    {
        pthread_create(&thr_id, NULL, &thread_worker, NULL);
    }

    sleep(60);
}