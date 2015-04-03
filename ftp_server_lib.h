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
#include <stdio.h>
#include <sys/select.h>
#include <pthread.h>
#include <sys/types.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>


#define SERV_PORT 9989
#define HOST_LEN 512
#define MAX_LEN 512

int get_server_socket_id();
void * ftp_do_ls(void * );

struct thread_parameter
{
	int socket_fd;
	char  cwd[100];
};

#endif
