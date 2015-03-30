#ifndef FTP_SERVER_LIB_H
#define FTP_SERVER_LIB_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>

#define SERV_PORT 9989
#define HOST_LEN 512

int get_server_socket_id();

#endif
