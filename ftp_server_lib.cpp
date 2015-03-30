#include "ftp_server_lib.h"

using namespace std;

int ftp_init()
{
	signal(SIGCHLD,SIG_IGN);
}

int get_server_socket_id()
{
	struct sockaddr_in saddr;
	struct hostent *hp;
	char hostname[HOST_LEN];
	int server_socket_id;

	server_socket_id = socket(AF_INET,SOCK_STREAM,0);
	if(server_socket_id == -1)
	{
		cout<<"socket error"<<endl;
		exit(-1);
	}

	bzero((void*)&saddr,sizeof(saddr));

	gethostname(hostname,HOST_LEN);
	hp = gethostbyname(hostname);

	bcopy((void*)hp->h_addr,(void*)&saddr.sin_addr,hp->h_length);
	saddr.sin_port = htons(SERV_PORT);
	saddr.sin_family = AF_INET;

	if(bind(server_socket_id,(struct sockaddr *)&saddr,sizeof(saddr)) != 0)
	{
		cout<<"bind error"<<endl;
		exit(-1);
	}

	if(listen(server_socket_id,10) != 0)
	{
		cout<<"listen error"<<endl;
		exit(-1);
	}

	return server_socket_id;
}

void do_loop(int server_socket_id)
{
	int client_socket_fd;
	while(true)
	{
		client_socket_fd = accept(server_socket_id,NULL,NULL);
		if(client_socket_fd == -1)
		{
			cout<<"accept error"<<endl;
			exit(-1);
		}
		struct sockaddr_in addr;
		socklen_t len;
		getpeername(client_socket_fd,(struct sockaddr*)&addr,&len);
		cout<<"accept a connection from "<<inet_ntoa(addr.sin_addr)<<" "<<ntohs(addr.sin_port)<<endl;
	}
}
