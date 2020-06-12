#ifndef __SERVER_H__
#define __SERVER_H__

#include "common.h"

#define USE_MAX_THREAD_NUM 20
#define USE_THREAD_TIMEOUT 600

#define RW_BUFFER_LENGTH 512
#define OPEN_MAX 100
#define LISTENQ 20
#define INFTIM 1000

typedef struct rwdata_tag {
    int fd;
    char buf[RW_BUFFER_LENGTH];
} rwdata_t;

int tcp_server(char *ip, int port);


#endif
