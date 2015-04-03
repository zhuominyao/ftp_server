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
		cout<<"li<F3>sten error"<<endl;
		exit(-1);
	}

	return server_socket_id;
}

void do_loop(int server_socket_id)
{
	fd_set rset,allset;
	int maxfd,maxi,n_ready,i;
	int client_socket_fd;
	int client[FD_SETSIZE];
	struct sockaddr_in addr;
	socklen_t len;
	bool new_connect = false;
	char cwd[FD_SETSIZE][100];



	maxfd = server_socket_id;
	maxi = -1;
	for(i = 0;i < FD_SETSIZE;i++)
	{
		client[i] = -1;
		strcpy(cwd[i],".");//将每一个fd的当前工作目录初始化为~
	}

	FD_ZERO(&allset);
	FD_SET(server_socket_id,&allset);

	while(true)
	{
		rset = allset;
		n_ready = select(maxfd+1,&rset,NULL,NULL,NULL);//阻塞直到监听的某一个fd有输入

		if(FD_ISSET(server_socket_id,&rset))//测试server_socket有没有输入,即有没有新的客户连接到server
		{
			client_socket_fd = accept(server_socket_id,NULL,NULL);
			new_connect = true;
		}
		else
			new_connect = false;
		
		if(client_socket_fd == -1)
		{
			cout<<"accept error"<<endl;
			exit(-1);
		}

		for(i = 0;i < FD_SETSIZE;i++)
		{
			if(client[i] < 0)
			{
				client[i] = client_socket_fd;
				break;
			}
		}

		if(i == FD_SETSIZE)//当连接数大于FD_SETSIZE时,程序退出
		{
			cout<<"客户端连接太多,关闭新连接的客户";
			close(client_socket_fd);
		}
		else
		{
			FD_SET(client_socket_fd,&allset);//将新的客户连接fd加入到allset中

			if(maxfd < client_socket_fd)//记录maxfd,因为select函数要用
				maxfd = client_socket_fd;
			if(i > maxi)//记录maxi
				maxi = i;

			//打印客户的ip和端口
			if(new_connect)
			{
				getpeername(client_socket_fd,(struct sockaddr*)&addr,&len);
				cout<<"accept a connection from "<<inet_ntoa(addr.sin_addr)<<" "<<ntohs(addr.sin_port)<<endl;
				cout<<"allocate "<<client_socket_fd<<" to it"<<endl;
				write(client_socket_fd,"220",4);	
			}
		}

		int socket_fd,n;
		for(i = 0;i <= maxi;i++)
		{
			char buffer[MAX_LEN];
			if((socket_fd = client[i]) < 0)
				continue;
			if(FD_ISSET(socket_fd,&rset))
			{
				if((n = read(socket_fd,buffer,MAX_LEN)) == 0)//客户端发出FIN
				{
					getpeername(socket_fd,(struct sockaddr*)&addr,&len);
					cout<<inet_ntoa(addr.sin_addr)<<" "<<ntohs(addr.sin_port)<<" disconnect"<<endl;
					cout<<"fd "<<socket_fd<<" release"<<endl;
					close(socket_fd);
					FD_CLR(socket_fd,&allset);
					client[i] = -1;
				}
				else
				{
					pthread_t tid;
					struct thread_parameter parameter;
					parameter.socket_fd = socket_fd;
					strcpy(parameter.cwd,cwd[i]);
					pthread_create(&tid,NULL,ftp_do_ls,&parameter);
					cout<<buffer<<endl;
				}
				if(--n_ready <= 0)
					break;
			}
		}
	}
}

void * ftp_do_ls(void * p)
{
	pthread_detach(pthread_self());
	struct thread_parameter * parameter = (struct thread_parameter *)p;
	cout<<"in thread"<<endl;
	cout<<"socket_fd:"<<parameter->socket_fd<<endl;
	cout<<"cwd:"<<parameter->cwd<<endl;
	
	DIR * dir;
	struct dirent * direntp;
	if((dir = opendir(parameter->cwd)) != NULL)
	{
		while((direntp = readdir(dir)) != NULL)
		{
			char absolute_path[100];
			strcpy(absolute_path,parameter->cwd);
			strcat(absolute_path,"/");
			strcat(absolute_path,direntp->d_name);
			cout<<absolute_path<<endl;
			write(parameter->socket_fd,absolute_path,strlen(absolute_path)+1);
		}
	}
	else
		cout<<"can not open "<<parameter->cwd<<endl;
}


