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
#include <sys/fcntl.h>

#define SERV_PORT 9989
#define HOST_LEN 512
#define MAX_LEN 512

int get_server_socket_id();
void * ftp_do_ls(void *);
void do_stat(FILE *,char *,char *);
void show_file_info(FILE *,char *,struct stat *,char *);
void mode_to_letters(int,char *);
char * uid_to_name(uid_t);
char * gid_to_name(gid_t);
void * ftp_do_cd(void *);
void * ftp_do_pwd(void *);
void * ftp_do_get(void *);
void * ftp_do_put(void *);
void * ftp_do_delete(void *);
int is_path_exist(char *,char *);


struct ls_parameter
{
	int socket_fd;
	char  cwd[100];
};

struct cd_parameter
{
	int socket_fd;
	char * cwd;
	char request_path[100];
};

struct pwd_parameter
{
	int socket_fd;
	char * cwd;
};

struct get_parameter
{
	int socket_fd;
	char filename[50];
	char * cwd;
};

struct put_parameter
{
	int socket_fd;
	char filename[50];
	char * cwd;
};

struct delete_parameter
{
	int socket_fd;
	char filename[50];
	char * cwd;
};
#endif
