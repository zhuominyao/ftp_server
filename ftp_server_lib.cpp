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
	cout<<"the hostname is :"<<hostname<<endl;

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
		strcpy(cwd[i],"/");//将每一个fd的当前工作目录初始化为当前工作路径
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

		if(i == FD_SETSIZE)//当连接数大于FD_SETSIZE时,关闭客户端连接
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
					strcpy(cwd[i],"/");
					close(socket_fd);
					FD_CLR(socket_fd,&allset);
					client[i] = -1;
				}
				else
				{
					buffer[n] = '\0';
					cout<<"get command:"<<buffer<<endl;
					pthread_t tid;
					if(strcmp(buffer,"ls") == 0)
					{
						struct ls_parameter ls_p;
						ls_p.socket_fd = socket_fd;
						strcpy(ls_p.cwd,cwd[i]);
						pthread_create(&tid,NULL,ftp_do_ls,&ls_p);
					}
					else if(strcmp(buffer,"cd") == 0)
					{
						char confirm[2] = "1";
						write(socket_fd,confirm,strlen(confirm));
						cout<<"enter cd"<<endl;
						char path[MAX_LEN];
						if((n = read(socket_fd,path,MAX_LEN)) == 0)
						{
							getpeername(socket_fd,(struct sockaddr*)&addr,&len);
							cout<<inet_ntoa(addr.sin_addr)<<" "<<ntohs(addr.sin_port)<<" disconnect"<<endl;
							cout<<"fd "<<socket_fd<<" release"<<endl;
							strcpy(cwd[i],"/");
							close(socket_fd);
							FD_CLR(socket_fd,&allset);
							client[i] = -1;
						}
						else
						{
							struct cd_parameter cd_p;
							path[n] = '\0';
							cout<<path<<endl;
							if(strcmp(path,"/") == 0)//特殊情况：输入的路径是根目录
							{
								strcpy(cwd[i],path);
								FILE * fw = fdopen(socket_fd,"w");
								fprintf(fw,"%d\n",1);
								fflush(fw);
							}
							else if(strcmp(path,".") == 0)//特殊情况：输入的路径是.
							{
								FILE * fw = fdopen(socket_fd,"w");
								fprintf(fw,"%d\n",1);
								fflush(fw);
							}
							else if(strcmp(path,"..") == 0)//特殊情况：输入的路径是..
							{
								if(strcmp(cwd[i],"/") == 0)//如果当前工作目录为根目录
								{
									FILE * fw = fdopen(socket_fd,"w");
									fprintf(fw,"%d\n",1);
									fflush(fw);
								}
								else//如果当前工作目录不为根目录
								{
									int j;
									for(j = strlen(cwd[i]);j >= 0;j--)//找到第一个'/'的位置
									{
										if(cwd[i][j] == '/')
											break;
									}
									if(j == 0)//如果当前目录的上一级目录是根目录
										strcpy(cwd[i],"/");
									else
										cwd[i][j] = '\0';
									cout<<cwd[i]<<endl;

									FILE * fw = fdopen(socket_fd,"w");
									fprintf(fw,"%d\n",1);
									fflush(fw);
								}
							}
							else
							{
								cd_p.socket_fd = socket_fd;
								cd_p.cwd = cwd[i];
								strcpy(cd_p.request_path,path);
								pthread_create(&tid,NULL,ftp_do_cd,&cd_p);
							}
						}
					}
					else if(strcmp(buffer,"pwd") == 0)
					{
						struct pwd_parameter pwd_p;
						pwd_p.socket_fd = socket_fd;
						pwd_p.cwd = cwd[i];
						pthread_create(&tid,NULL,ftp_do_pwd,&pwd_p);
					}
					else if(strcmp(buffer,"get") == 0)
					{
						char confirm[2] = "1";
						write(socket_fd,confirm,strlen(confirm));
						char filename[MAX_LEN];
						int n;
						if((n = read(socket_fd,filename,MAX_LEN)) == 0)
						{
							getpeername(socket_fd,(struct sockaddr*)&addr,&len);
							cout<<inet_ntoa(addr.sin_addr)<<" "<<ntohs(addr.sin_port)<<" disconnect"<<endl;
							cout<<"fd "<<socket_fd<<" release"<<endl;
							strcpy(cwd[i],"/");
							close(socket_fd);
							FD_CLR(socket_fd,&allset);
							client[i] = -1;
						}
						else
						{
							filename[n] = '\0';
							cout<<filename<<endl;
							struct get_parameter get_p;
							get_p.socket_fd = socket_fd;
							strcpy(get_p.filename,filename);
							get_p.cwd = cwd[i];
							pthread_create(&tid,NULL,ftp_do_get,&get_p);
						}
					}
				}
				if(--n_ready <= 0)
					break;
			}
		}
	}
}

void * ftp_do_get(void * p)
{
	pthread_detach(pthread_self());

	struct get_parameter * parameter = (struct get_parameter*)p;
	char root[3] = "/";
	int statue = -1;
	FILE * fw;
	fw = fdopen(parameter->socket_fd,"w");

	cout<<"filename:"<<parameter->filename<<endl;
	cout<<"cwd:"<<parameter->cwd<<endl;

	if(parameter->filename[0] == '/')
		statue = is_path_exist(root,parameter->filename+1);
	else
		statue = is_path_exist(parameter->cwd,parameter->filename);

	cout<<statue<<endl;
	fprintf(fw,"%d\n",statue);
	fflush(fw);
}

void * ftp_do_pwd(void * p)
{
	pthread_detach(pthread_self());

	struct pwd_parameter * parameter = (struct pwd_parameter*)p;
	FILE * fw = fdopen(parameter->socket_fd,"w");

	fprintf(fw,"%s\n",parameter->cwd);
	fflush(fw);
}

void * ftp_do_ls(void * p)
{
	pthread_detach(pthread_self());
	
	struct ls_parameter* parameter = (struct ls_parameter*)p;
	
	cout<<"in thread"<<endl;
	cout<<"socket_fd:"<<parameter->socket_fd<<endl;
	cout<<"cwd:"<<parameter->cwd<<endl;
	
	DIR * dir;
	struct dirent * direntp;
	
	if((dir = opendir(parameter->cwd)) != NULL)
	{
		FILE * fw = fdopen(parameter->socket_fd,"w");
		while((direntp = readdir(dir)) != NULL)
		{
		//	if(strcmp(direntp->d_name,".") == 0 || strcmp(direntp->d_name,"..") == 0)
		//		continue;
			char absolute_path[100];
			strcpy(absolute_path,parameter->cwd);
			if(strcmp(absolute_path,"/") != 0)
				strcat(absolute_path,"/");
			strcat(absolute_path,direntp->d_name);
			fprintf(fw,"%-50s",absolute_path);
			do_stat(fw,absolute_path,direntp->d_name);
			fflush(fw);
		}
		fprintf(fw,"endoffile\n");
		fflush(fw);
		closedir(dir);
	}
	else
		cout<<"can not open "<<parameter->cwd<<endl;
}

void * ftp_do_cd(void * p)
{
	pthread_detach(pthread_self());
	struct cd_parameter * parameter= (struct cd_parameter *)p;

	cout<<"in thread"<<endl;
	cout<<"socket_fd:"<<parameter->socket_fd<<endl;
	cout<<"cwd:"<<parameter->cwd<<endl;
	cout<<"request path:"<<parameter->request_path<<endl;

	int statue = -1;
	char root[5] = "/";
	FILE * fw = fdopen(parameter->socket_fd,"w");

	if(parameter->request_path[0] == '/')
		statue = is_path_exist(root,parameter->request_path+1);
	else
		statue = is_path_exist(parameter->cwd,parameter->request_path);

	cout<<statue<<endl;
	if(statue == 1)
	{
		if(parameter->request_path[0] == '/')//如果请求的是绝对路径
		{
			strcpy(parameter->cwd,parameter->request_path);
			cout<<"new cwd:"<<parameter->cwd<<endl;
		}
		else//如果请求的是相对路径
		{
			if(parameter->cwd[strlen(parameter->cwd)-1] != '/')
				strcat(parameter->cwd,"/");
			strcat(parameter->cwd,parameter->request_path);
			cout<<"new cwd:"<<parameter->cwd<<endl;
		}
		fprintf(fw,"%d\n",statue);
		fflush(fw);
	}
	else if(statue == 2)
	{
		cout<<"the request path exist,but is not a directory"<<endl;
		fprintf(fw,"%d\n",statue);
		fflush(fw);
	}
	else if(statue == 3)
	{
		cout<<"the request path is not exist"<<endl;
		fprintf(fw,"%d\n",statue);
		fflush(fw);
	}
}

int is_path_exist(char * root,char * request_path)
{
	cout<<"root:"<<root<<endl;
	cout<<"request_path:"<<request_path<<endl;

	char absolute_path[MAX_LEN];
	struct stat s;
	int stat_result;

	strcpy(absolute_path,root);
	strcat(absolute_path,"/");
	strcat(absolute_path,request_path);
	cout<<"absolute_path:"<<absolute_path<<endl;

	stat_result = lstat(absolute_path,&s);
	if(stat_result == 0)
	{
		if(S_ISDIR(s.st_mode))
			return 1;//请求的路径存在且为文件夹
		else
			return 2;//请求的路径存在但不为文件夹
	}
	else
		return 3;//请求的路径不存在

}

/*int is_path_exist(char * root,char * request_path)
{
	cout<<"root:"<<root<<endl;
	cout<<"request_path:"<<request_path<<endl;
	DIR * dir;
	struct dirent * direntp;

	if((dir = opendir(root)) != NULL)
	{
		while((direntp = readdir(dir)) != NULL)
		{
			if(strcmp(direntp->d_name,request_path) == 0)
			{
				struct stat s;
				char absolute_path[MAX_LEN];

				strcpy(absolute_path,root);
				strcat(absolute_path,"/");
				strcat(absolute_path,request_path);

				lstat(absolute_path,&s);
				if(S_ISDIR(s.st_mode))
				{

					return 1;//请求的路径存在且为文件夹
				}
				else
					return 2;//请求的路径存在但不为文件夹
			}
		}
		return 3;//请求的路径不存在
	}
}*/

void do_stat(FILE * fw,char * absolute_path,char * filename)
{
	struct stat s;
	if(lstat(absolute_path,&s) == -1)
		cout<<"lstat error"<<endl;
	else
		show_file_info(fw,absolute_path,&s,filename);
}

void show_file_info(FILE * fw,char * absolute_filename,struct stat * info,char * filename)  
{  
	char mode[11];  
	mode_to_letters(info->st_mode,mode);  
	fprintf(fw,"%-10s ",mode);  
	fprintf(fw,"%-10d ",info->st_nlink);  
	fprintf(fw,"%-10s ",uid_to_name(info->st_uid));  
	fprintf(fw,"%-10s ",gid_to_name(info->st_gid));  
	fprintf(fw,"%-10d ",(int)info->st_size);  
	fprintf(fw,"%.12s ",4+ctime(&info->st_mtime));  
	fprintf(fw,"\n");
}  
  
void mode_to_letters(int mode,char * c_mode)  
{  
	strcpy(c_mode,"----------");  
		  
	if(S_ISDIR(mode))  
		c_mode[0] = 'd';  
	if(S_ISCHR(mode))  
		c_mode[0] = 'c';  
	if(S_ISBLK(mode))  
		c_mode[0] = 'b';  
					      
	if(mode & S_IRUSR)  
		c_mode[1] = 'r';  
	if(mode & S_IWUSR)  
		c_mode[2] = 'w';  
	if(mode & S_IXUSR)  
		c_mode[3] = 'x';  
								  
	if(mode & S_IRGRP)  
		c_mode[4] = 'r';  
	if(mode & S_IWGRP)  
		c_mode[5] = 'w';  
	if(mode & S_IXGRP)  
		c_mode[6] = 'x';  
													  
	if(mode & S_IROTH)  
		c_mode[7] = 'r';  
	if(mode & S_IWOTH)  
		c_mode[8] = 'w';  
	if(mode & S_IXOTH)  
		c_mode[9] = 'x';  
																		  
	if(mode & S_ISUID)  
		c_mode[3] = 's';  
	if(mode & S_ISGID)  
		c_mode[6] = 's';  
	if(mode & S_ISVTX)  
		c_mode[9] = 's';  
}  
  
char * uid_to_name(uid_t uid)  
{  
	struct passwd * passwd_pointer;  
	passwd_pointer = getpwuid(uid);  
	return passwd_pointer->pw_name;  
}  
  
char * gid_to_name(gid_t gid)  
{        
	struct group * group_pointer;  
	static char numstr[10];  
	if((group_pointer = getgrgid(gid)) == NULL)  
	{  
		sprintf(numstr,"%d",gid);  
		return numstr;  
	}  
	return group_pointer->gr_name;  
}
