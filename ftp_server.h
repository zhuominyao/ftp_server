#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <stdio.h>

#define SERV_PORT 9989
#define HOST_LEN 512

int get_server_socket_id();
int ftp_init();
int do_loop(int);

#endif
