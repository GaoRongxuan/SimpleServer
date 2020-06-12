#include <stdio.h>

#include "server.h"
#include "thrmgr.h"

int main(int argc, char* argv[])
{

    char *ip = "127.0.0.1";
    int port = 8890;

    thrmgr_init();

    tcp_server(ip, port);
    return 0;
}

